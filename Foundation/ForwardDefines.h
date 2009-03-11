// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_Framework_ForwardDefines_h
#define incl_Framework_ForwardDefines_h

#include <Poco/Logger.h>

namespace Foundation
{
    class ModuleManager;
    class ComponentManager;
    class ComponentFactoryInterface;
    class ServiceManager;

    typedef boost::shared_ptr<ModuleManager> ModuleManagerPtr;
    typedef boost::shared_ptr<ComponentManager> ComponentManagerPtr;
    typedef boost::shared_ptr<ComponentFactoryInterface> ComponentFactoryInterfacePtr;
    typedef boost::shared_ptr<ServiceManager> ServiceManagerPtr;
    
    //! Use root logging only foundation in classes.
    static void RootLogFatal(const std::string &msg)
    {
        Poco::Logger::get("Foundation").fatal(msg);
    }
    static void RootLogCritical(const std::string &msg)
    {
        Poco::Logger::get("Foundation").critical(msg);
    }
    static void RootLogError(const std::string &msg)
    {
        Poco::Logger::get("Foundation").error(msg);
    }
    static void RootLogWarning(const std::string &msg)
    {
        Poco::Logger::get("Foundation").warning(msg);
    }
    static void RootLogNotice(const std::string &msg)
    {
        Poco::Logger::get("Foundation").notice(msg);
    }
    static void RootLogInfo(const std::string &msg)
    {
        Poco::Logger::get("Foundation").information(msg);
    }
    static void RootLogTrace(const std::string &msg)
    {
        Poco::Logger::get("Foundation").trace(msg);
    }
}

#endif
