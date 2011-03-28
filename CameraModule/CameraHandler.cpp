//$ HEADER_NEW_FILE $
// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "CameraHandler.h"
#include "Quaternion.h"

#include <SceneManager.h>
#include <EC_Placeable.h>
#include <EC_OgreCamera.h>
#include <EC_Mesh.h>
#include <EC_RttTarget.h>
#include <EC_OgreCustomObject.h>

#include <QUuid>

#include <QDebug>

namespace Camera
{
    CameraHandler::CameraHandler(Foundation::Framework *framework, QObject *parent) :
        QObject(parent),
        framework_(framework),
        zoom_acceleration_(0.001f),
        camera_type_(Perspective),
        projection_type_(Ogre::PT_PERSPECTIVE),
        polygon_mode_(Ogre::PM_SOLID),
        min_zoom_distance_(3),
        max_zoom_distance_(100),
        target_entity_(0)
    {                          
    }

    CameraHandler::~CameraHandler()
    {
        DestroyCamera();
    }    

    bool CameraHandler::CreateCamera(Scene::SceneManager *scene)
    {
        if (!cam_entity_)
        {
            scene_ = scene;
            //create camera entity
            if (!scene_)
                return false;

            cam_entity_ = scene_->CreateEntity(scene_->GetNextFreeId());            
            if (!cam_entity_.get())
                return false;

            cam_entity_->AddComponent(framework_->GetComponentManager()->CreateComponent(EC_Placeable::TypeNameStatic()));
            cam_entity_->AddComponent(framework_->GetComponentManager()->CreateComponent(EC_OgreCamera::TypeNameStatic()));
            cam_entity_->AddComponent(framework_->GetComponentManager()->CreateComponent(EC_RttTarget::TypeNameStatic()));
            scene_->EmitEntityCreated(cam_entity_);
            ComponentPtr component_placeable = cam_entity_->GetComponent(EC_Placeable::TypeNameStatic());
            EC_OgreCamera *ec_camera = cam_entity_->GetComponent<EC_OgreCamera>().get();

            SetCameraType(camera_type_);
            SetCameraProjection(projection_type_);
            SetCameraWireframe(polygon_mode_);

            if (!component_placeable.get() || !ec_camera)
                return false;          

            ec_camera->SetPlaceable(component_placeable);
            return true;
        }            
        return false;
    }

    void CameraHandler::DestroyCamera()
    {
        if (scene_)
            scene_->RemoveEntity(cam_entity_->GetId());
    }

    bool CameraHandler::FocusToEntity(Scene::Entity *entity, Vector3df offset)
    {
        float max_distance = 0;
        bool focus_completed = false;
        if (cam_entity_)
        {                
            // Get placeable from both focus entity and our camera id entity
            EC_Placeable *entity_ec_placeable = entity->GetComponent<EC_Placeable>().get();
            EC_Placeable *cam_ec_placeable = cam_entity_->GetComponent<EC_Placeable>().get();
            EC_OgreCamera *cam_ec_camera = cam_entity_->GetComponent<EC_OgreCamera>().get();
            
            if (!entity_ec_placeable || !cam_ec_placeable || !cam_ec_camera)
                return focus_completed;

            EC_Mesh *entity_mesh = entity->GetComponent<EC_Mesh>().get();
            EC_OgreCustomObject *entity_custom_object = entity->GetComponent<EC_OgreCustomObject>().get();

            Vector3df position_vector = entity_ec_placeable->transform.Get().position;
            Vector3df position_offset;
            Vector3df look_at;

            Vector3df bounding_min;
            Vector3df bounding_max;
            Vector3df der_size_vector;
            Ogre::Vector3 derived_scale;

            if (entity_mesh)
            {
                if (!entity_mesh->GetEntity())
                    return false;

                entity_mesh->GetBoundingBox(bounding_min, bounding_max);
                derived_scale = entity_mesh->GetEntity()->getParentNode()->_getDerivedScale();
                der_size_vector = Vector3df(derived_scale.x, derived_scale.y, derived_scale.z) * (bounding_max - bounding_min);                                
            }
            else if (entity_custom_object)
            {
                if (!entity_custom_object->GetEntity())
                    return false;

                entity_custom_object->GetBoundingBox(bounding_min, bounding_max);
                derived_scale = entity_custom_object->GetEntity()->getParentNode()->_getDerivedScale();
                der_size_vector = Vector3df(derived_scale.x, derived_scale.y, derived_scale.z) * (bounding_max - bounding_min);
                                
            }
            else
                return focus_completed;
            
            if (der_size_vector.x > max_distance)
                max_distance = der_size_vector.x;
            if (der_size_vector.y > max_distance)
                max_distance = der_size_vector.y;
            if (der_size_vector.z > max_distance)
                max_distance = der_size_vector.z;

            if (cam_ec_camera->GetCamera()->getProjectionType() == Ogre::PT_ORTHOGRAPHIC)
                cam_ec_camera->GetCamera()->setOrthoWindowHeight(max_distance);
                
            if (camera_type_ == Perspective)
            {
                position_offset = Vector3df(der_size_vector.x, -der_size_vector.y, der_size_vector.y);
                position_offset = entity_ec_placeable->GetOrientation()* position_offset;
                focus_completed = true;
            }
            else 
            {
                if (cam_ec_camera->GetCamera()->getProjectionType() == Ogre::PT_ORTHOGRAPHIC)
                    cam_ec_camera->GetCamera()->setOrthoWindowHeight(max_distance);
                position_offset = max_distance * z_vector;
                focus_completed = true;
            }

            if (focus_completed)
            {
                look_at = Vector3df(position_vector.x, position_vector.y, position_vector.z);
                Transform trans = cam_ec_placeable->transform.Get();
                trans.position = position_vector + position_offset;
                cam_ec_placeable->transform.Set(trans, AttributeChange::LocalOnly);
                cam_ec_placeable->LookAt(look_at);
                target_entity_ = entity;
            }            
        }
        return focus_completed;
    }

    void CameraHandler::Move(qreal x, qreal y)
    {
        if (cam_entity_)
        {
            EC_Placeable *cam_ec_placeable = cam_entity_->GetComponent<EC_Placeable>().get();
            if (!cam_ec_placeable)
                return;

            Vector3df pos = cam_ec_placeable->transform.Get().position;
            EC_OgreCamera *cam_ec_camera = cam_entity_->GetComponent<EC_OgreCamera>().get();

            //modify x and y to world scale
            //if projection type is PT_ORTHOGRAPHIC
            if ((cam_ec_camera->GetCamera()->getProjectionType() == Ogre::PT_ORTHOGRAPHIC) && (camera_type_ != Perspective))
            {
                float world_width = cam_ec_camera->GetCamera()->getOrthoWindowWidth();
                float world_height = cam_ec_camera->GetCamera()->getOrthoWindowHeight();             
                x = x * world_width;
                y = y * world_height;
            }

            if (camera_type_== Perspective)
            {
                EC_Placeable *entity_ec_placeable = target_entity_->GetComponent<EC_Placeable>().get();
                if (!entity_ec_placeable)
                    return;
                RotateCamera(entity_ec_placeable->transform.Get().position, x, y);
            }
            else
            {
                pos += x_vector * x;
                pos += y_vector * y;
                Transform trans = cam_ec_placeable->transform.Get();
                trans.position = pos;
                cam_ec_placeable->transform.Set(trans,AttributeChange::LocalOnly);

            }            
        }
    }

    void CameraHandler::RotateCamera(Vector3df pivot, qreal x, qreal y)
    {
        if (cam_entity_)
        {            
            EC_OgreCamera *ec_camera = cam_entity_->GetComponent<EC_OgreCamera>().get();
            EC_Placeable *cam_ec_placeable = cam_entity_->GetComponent<EC_Placeable>().get();
            if (!ec_camera || !cam_ec_placeable)
                return;

            Vector3df pos = cam_ec_placeable->transform.Get().position;
            //GetPosition();

            Vector3df dir(pos-pivot);
            Quaternion quat(-x,cam_ec_placeable->GetLocalYAxis());
            quat *= Quaternion(-y, cam_ec_placeable->GetLocalXAxis());
            dir = quat * dir;

            Vector3df new_pos(pivot+dir);
            Transform trans = cam_ec_placeable->transform.Get();
            trans.position = new_pos;
            cam_ec_placeable->transform.Set(trans,AttributeChange::LocalOnly);
            cam_ec_placeable->LookAt(pivot);
        }
    }

    bool CameraHandler::Zoom(qreal delta, Qt::KeyboardModifiers modifiers)
    {
        bool zoomed = false;
        
        if (cam_entity_)
        {
            //get camera entity placeable component
            EC_Placeable *placeable = cam_entity_->GetComponent<EC_Placeable>().get();
            EC_OgreCamera *cam_ec_camera = cam_entity_->GetComponent<EC_OgreCamera>().get();

            //get target entity placeable component
            if ((!target_entity_) && (!placeable) && (!cam_ec_camera))
                return false;

            

            //move near clip distance
            if (modifiers & Qt::AltModifier)
            {                
                float new_near_clip = cam_ec_camera->GetNearClip()+delta*zoom_acceleration_;
                if (new_near_clip > 0)
                    cam_ec_camera->SetNearClip(new_near_clip);
            }
            else
            {
                EC_Placeable *target_placeable = target_entity_->GetComponent<EC_Placeable>().get();

                if (!target_placeable)
                    return false;

                //get target and cam position
                Vector3df point = target_placeable->GetPosition();
                Vector3df pos = placeable->GetPosition();
                //get direction vector
                Vector3df dir = point-pos;
                Vector3df distance = dir;
                
                //modify direction vector with wheel delta and zoom aceleration constants
                dir *=delta*zoom_acceleration_;
                               
                switch (camera_type_)
                {
                    case Perspective:
                    {
                        //something fishy, even if we check that we never go beyond min/max, we still might end up there and zoom will be disabled. So we check the also
                        //if were zooming in or out.
                        if ((delta>0 && (distance.getLength()+dir.getLength() > min_zoom_distance_)) 
                           || (delta<0 && (distance.getLength()+dir.getLength() < max_zoom_distance_)))
                        {
                            pos = placeable->GetPosition() + dir;
                            zoomed = true;
                        }
                        break;
                    }
                    case Front:
                    case Back:
                        {
                            //check with min and max distance if zoom in x is posible
                            if((delta>0 && (abs(distance.x) + dir.x > min_zoom_distance_)) 
                                || (delta<0 && (abs(distance.x) + dir.x < max_zoom_distance_)))
                            {
                                if (cam_ec_camera->GetCamera()->getProjectionType() == Ogre::PT_ORTHOGRAPHIC) 
                                {
                                    float new_height = cam_ec_camera->GetCamera()->getOrthoWindowHeight()+dir.x*z_signed;
                                    if (new_height > 0)
                                        cam_ec_camera->GetCamera()->setOrthoWindowHeight(new_height);   
                                }
                                pos.x += dir.x;
                                zoomed = true;
                            }                        
                            break;
                        }
                    case Left:  
                    case Rigth:
                        {      
                            //check with min and max distance if zoom in y is posible
                            if((delta>0 && (abs(distance.y) + dir.y > min_zoom_distance_)) 
                                || (delta<0 && (abs(distance.y) + dir.y < max_zoom_distance_)))
                            {
                                if (cam_ec_camera->GetCamera()->getProjectionType() == Ogre::PT_ORTHOGRAPHIC) 
                                {
                                    float new_height = cam_ec_camera->GetCamera()->getOrthoWindowHeight()+dir.y*z_signed;
                                    if (new_height > 0)
                                        cam_ec_camera->GetCamera()->setOrthoWindowHeight(new_height);   
                                }
                                pos.y += dir.y;
                                zoomed = true;
                            }                        
                            break;
                        }
                    case Top:
                    case Bottom:
                        {                                           
                            //check with min and max distance if zoom in z is posible
                            if((delta>0 && (abs(distance.z) + dir.z > min_zoom_distance_)) 
                                || (delta<0 && (abs(distance.z) + dir.z < max_zoom_distance_)))
                            {
                                if (cam_ec_camera->GetCamera()->getProjectionType() == Ogre::PT_ORTHOGRAPHIC) 
                                {
                                    float new_height = cam_ec_camera->GetCamera()->getOrthoWindowHeight()+dir.z*z_signed;
                                    if (new_height > 0)
                                        cam_ec_camera->GetCamera()->setOrthoWindowHeight(new_height);   
                                }
                                pos.z += dir.z;
                                zoomed = true;
                            }                        
                            break;
                        }
                    }
                //if zoom is posible set cam entity new position
                if (zoomed)
                {                     
                    Transform trans = placeable->transform.Get();
                    trans.position = pos;
                    placeable->transform.Set(trans,AttributeChange::LocalOnly);
                }
            }
        }
        return zoomed;
    }

    QPixmap CameraHandler::RenderCamera(QSize image_size)
    {
        if (cam_entity_)
        {
            // Get camera
            QImage captured_pixmap(image_size, QImage::Format_ARGB32_Premultiplied);
            captured_pixmap.fill(Qt::gray);

            // Get the camera ec
            EC_RttTarget *ec_rtt = cam_entity_->GetComponent<EC_RttTarget>().get();
            if (!ec_rtt)
                return QPixmap::fromImage(captured_pixmap);

            Ogre::uchar* pixelData = ec_rtt->GetRawTexture(image_size.width(), image_size.height());
            captured_pixmap = QImage(pixelData, image_size.width(), image_size.height(), QImage::Format_ARGB32_Premultiplied);               
           
            // Return image as a QPixmap
            return QPixmap::fromImage(captured_pixmap);
        }
        else
            return 0;
       
    }      
   
    void CameraHandler::SetCoordinates()
    {
        switch (camera_type_)
        {
            case Perspective:
            {
                break;
            }
            case Front:
            {      
                x_vector = Vector3df(0,-1,0);
                y_vector = Vector3df(0,0,1);   
                z_vector = Vector3df(1,0,0);   
                z_signed = 1;
                break;
            }
            case Back:
            {      
                x_vector = Vector3df(0,1,0);
                y_vector = Vector3df(0,0,1);                
                z_vector = Vector3df(-1,0,0);   
                z_signed = -1;
                break;
            }
            case Left:
            {      
                x_vector = Vector3df(-1,0,0);
                y_vector = Vector3df(0,0,1);
                z_vector = Vector3df(0,-1,0);   
                z_signed = -1;
                break;
            }
            case Rigth:
            {      
                x_vector = Vector3df(1,0,0);
                y_vector = Vector3df(0,0,1);                
                z_vector = Vector3df(0,1,0);   
                z_signed = 1;
                break;
            }
            case Top:
            {      
                x_vector = Vector3df(1,0,0);
                y_vector = Vector3df(0,-1,0);                
                z_vector = Vector3df(0,0,1);   
                z_signed = 1;
                break;
            }
            case Bottom:
            {      
                x_vector = Vector3df(1,0,0);
                y_vector = Vector3df(0,1,0);                
                z_vector = Vector3df(0,0,-1);   
                z_signed = -1;
                break;
            }
        }   

    }

    void CameraHandler::SetCameraType(int camera_type)
    {        
        //CameraHandler::CameraType
        camera_type_ = (CameraHandler::CameraType) camera_type;
        SetCoordinates();
        if (target_entity_)
            FocusToEntity(target_entity_);

    }

    void CameraHandler::SetCameraProjection(int projection)
    {
        projection_type_ = (Ogre::ProjectionType) projection;
        if (cam_entity_)
        {
            EC_OgreCamera *ec_camera = cam_entity_->GetComponent<EC_OgreCamera>().get();
            //Ogre::ProjectionType
            ec_camera->GetCamera()->setProjectionType(projection_type_);
            if (target_entity_)
                FocusToEntity(target_entity_);
        }
    }

    void CameraHandler::SetCameraWireframe(bool state)
    {
        if (state == false)
            polygon_mode_ = Ogre::PM_SOLID;
        else
            polygon_mode_ = Ogre::PM_WIREFRAME;

        if (cam_entity_)
        {
            EC_OgreCamera *ec_camera = cam_entity_->GetComponent<EC_OgreCamera>().get();
            //Qt::CheckState: 0: unchecked, 1: partial checked, 2: checked
            ec_camera->GetCamera()->setPolygonMode(polygon_mode_);
        }
    }

    void CameraHandler::SetCameraWireframe(Ogre::PolygonMode polygon_mode)
    {
        polygon_mode_ = polygon_mode;

        if (cam_entity_)
        {
            EC_OgreCamera *ec_camera = cam_entity_->GetComponent<EC_OgreCamera>().get();
            //Qt::CheckState: 0: unchecked, 1: partial checked, 2: checked
            ec_camera->GetCamera()->setPolygonMode(polygon_mode_);
        }
    }
    bool CameraHandler::IsWireframeEnabled() 
    {
        return (polygon_mode_ == Ogre::PM_WIREFRAME);
    }
}