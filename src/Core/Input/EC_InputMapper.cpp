/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   EC_InputMapper.cpp
 *  @brief  Registers an InputContext from the Input API and uses it to translate
 *          given set of keys to Entity Actions on the entity the component is part of.
 */

#include "DebugOperatorNew.h"

#include "EC_InputMapper.h"
#include "InputAPI.h"

#include "Framework.h"
#include "IAttribute.h"
#include "AttributeMetadata.h"
#include "Entity.h"
#include "LoggingFunctions.h"

#include "MemoryLeakCheck.h"

EC_InputMapper::EC_InputMapper(Scene* scene):
    IComponent(scene),
    contextName(this, "Input context name", "EC_InputMapper"),
    contextPriority(this, "Input context priority", 90),
    takeKeyboardEventsOverQt(this, "Take keyboard events over Qt", false),
    takeMouseEventsOverQt(this, "Take mouse events over Qt", false),
    mappings(this, "Mappings"),
    executionType(this, "Action execution type", 1),
    modifiersEnabled(this, "Key modifiers enable", true),
    enabled(this, "Enable actions", true),
    keyrepeatTrigger(this, "Trigger on keyrepeats", true)
{
    static AttributeMetadata executionAttrData;
    static bool metadataInitialized = false;
    if(!metadataInitialized)
    {
        executionAttrData.enums[EntityAction::Local] = "Local";
        executionAttrData.enums[EntityAction::Server] = "Server";
        executionAttrData.enums[EntityAction::Server | EntityAction::Local] = "Local+Server";
        executionAttrData.enums[EntityAction::Peers] = "Peers";
        executionAttrData.enums[EntityAction::Peers | EntityAction::Local] = "Local+Peers";
        executionAttrData.enums[EntityAction::Peers | EntityAction::Server] = "Local+Server";
        executionAttrData.enums[EntityAction::Peers | EntityAction::Server | EntityAction::Local] = "Local+Server+Peers";
        metadataInitialized = true;
    }
    executionType.SetMetadata(&executionAttrData);
    
    connect(this, SIGNAL(AttributeChanged(IAttribute *, AttributeChange::Type)),
        SLOT(HandleAttributeUpdated(IAttribute *, AttributeChange::Type)));

    inputContext = GetFramework()->Input()->RegisterInputContext(contextName.Get().toStdString().c_str(), contextPriority.Get());
    inputContext->SetTakeKeyboardEventsOverQt(takeKeyboardEventsOverQt.Get());
    inputContext->SetTakeMouseEventsOverQt(takeMouseEventsOverQt.Get());
    connect(inputContext.get(), SIGNAL(KeyEventReceived(KeyEvent *)), SLOT(HandleKeyEvent(KeyEvent *)));
    connect(inputContext.get(), SIGNAL(MouseEventReceived(MouseEvent *)), SLOT(HandleMouseEvent(MouseEvent *)));
}

EC_InputMapper::~EC_InputMapper()
{
    inputContext.reset();
}

void EC_InputMapper::RegisterMapping(const QKeySequence &keySeq, const QString &action, int eventType, int executionType)
{
    ActionInvocation invocation;
    invocation.name = action;
    invocation.executionType = executionType;
    actionInvokationMappings[qMakePair(keySeq, (KeyEvent::EventType)eventType)] = invocation;
}

void EC_InputMapper::RegisterMapping(const QString &keySeq, const QString &action, int eventType, int executionType)
{
    ActionInvocation invocation;
    invocation.name = action;
    invocation.executionType = executionType;
    QKeySequence key(keySeq);
    if (!key.isEmpty())
        actionInvokationMappings[qMakePair(key, (KeyEvent::EventType)eventType)] = invocation;
}

void EC_InputMapper::RemoveMapping(const QKeySequence &keySeq, int eventType)
{
    ActionInvocationMap::iterator it = actionInvokationMappings.find(qMakePair(keySeq, (KeyEvent::EventType)eventType));
    if (it != actionInvokationMappings.end())
        actionInvokationMappings.erase(it);
}

void EC_InputMapper::RemoveMapping(const QString &keySeq, int eventType)
{
    ActionInvocationMap::iterator it = actionInvokationMappings.find(qMakePair(QKeySequence(keySeq), (KeyEvent::EventType)eventType));
    if (it != actionInvokationMappings.end())
        actionInvokationMappings.erase(it);
}

void EC_InputMapper::HandleAttributeUpdated(IAttribute *attribute, AttributeChange::Type change)
{
    if(attribute == &contextName || attribute == &contextPriority)
    {
        inputContext.reset();
        inputContext = GetFramework()->Input()->RegisterInputContext(contextName.Get().toStdString().c_str(), contextPriority.Get());
        connect(inputContext.get(), SIGNAL(KeyEventReceived(KeyEvent *)), SLOT(HandleKeyEvent(KeyEvent *)));
        connect(inputContext.get(), SIGNAL(MouseEventReceived(MouseEvent *)), SLOT(HandleMouseEvent(MouseEvent *)));
    }
    else if(attribute == &takeKeyboardEventsOverQt)
    {
        inputContext->SetTakeKeyboardEventsOverQt(takeKeyboardEventsOverQt.Get());
    }
    else if(attribute == &takeMouseEventsOverQt)
    {
        inputContext->SetTakeMouseEventsOverQt(takeMouseEventsOverQt.Get());
    }
    else if(attribute == &mappings)
    {
    }
}

void EC_InputMapper::HandleKeyEvent(KeyEvent *e)
{
    if (!enabled.Get())
        return;

    // Do not act on already handled key events.
    if (!e || e->handled)
        return;
    
    if (!keyrepeatTrigger.Get())
    {
        // Now we do not repeat key pressed events.
        if (e != 0 && e->eventType == KeyEvent::KeyPressed &&  e->keyPressCount > 1)
            return;
    }

    ActionInvocationMap::iterator it;
    if (modifiersEnabled.Get())
        it = actionInvokationMappings.find(qMakePair(QKeySequence(e->keyCode | e->modifiers), e->eventType));
    else
        it = actionInvokationMappings.find(qMakePair(QKeySequence(e->keyCode), e->eventType));
    if (it == actionInvokationMappings.end())
        return;

    Entity *entity = ParentEntity();
    if (!entity)
    {
        LogWarning("Parent entity not set. Cannot execute action.");
        return;
    }

    ActionInvocation& invocation = it.value();
    QString &action = invocation.name;
    int execType = invocation.executionType;
    // If zero execution type, use default
    if (!execType)
        execType = executionType.Get();
    
    // If the action has parameters, parse them from the action string.
    int idx = action.indexOf('(');
    if (idx != -1)
    {
        QString act = action.left(idx);
        QString parsedAction = action.mid(idx + 1);
        parsedAction.remove('(');
        parsedAction.remove(')');
        QStringList parameters = parsedAction.split(',');
        entity->Exec((EntityAction::ExecType)execType, act, parameters);
    }
    else
        entity->Exec((EntityAction::ExecType)execType, action);
}

void EC_InputMapper::HandleMouseEvent(MouseEvent *e)
{
    if (!enabled.Get())
        return;
    if (!ParentEntity())
        return;
    
    /// \todo this hard coding of look button logic is not nice!
    if ((e->IsButtonDown(MouseEvent::RightButton)) && (!GetFramework()->Input()->IsMouseCursorVisible()))
    {
        if (e->relativeX != 0)
            ParentEntity()->Exec((EntityAction::ExecType)executionType.Get(), "MouseLookX" , QString::number(e->relativeX));
        if (e->relativeY != 0)
            ParentEntity()->Exec((EntityAction::ExecType)executionType.Get(), "MouseLookY" , QString::number(e->relativeY));
    }
    if (e->relativeZ != 0 && e->relativeZ != -1) // For some reason this is -1 without scroll
        ParentEntity()->Exec((EntityAction::ExecType)executionType.Get(), "MouseScroll" , QString::number(e->relativeZ));
}
