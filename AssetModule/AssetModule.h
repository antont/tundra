// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_AssetModule_h
#define incl_AssetModule_h

#include "IModule.h"
#include "ModuleLoggingFunctions.h"

#include <QObject>
#include "ConsoleCommandUtils.h"

struct UserConnectedResponseData;
class UserConnection;

#include "IAssetProvider.h"
#include "AssetModuleApi.h"

namespace Asset
{
    /** \defgroup AssetModuleClient AssetModule Client Interface
        This page lists the public interface of the AssetModule,
        which consists of implementing Foundation::AssetServiceInterface and
        Foundation::AssetInterface

        For details on how to use the public interface, see \ref AssetModule "Using the asset module".
    */

    //! Asset module.
    class ASSET_MODULE_API AssetModule : public QObject, public IModule
    {
        Q_OBJECT

    public:
        AssetModule();
        virtual ~AssetModule();

        virtual void Initialize();
        virtual void PostInitialize();

        MODULE_LOGGING_FUNCTIONS

        //! callback for console command
        ConsoleCommandResult ConsoleRequestAsset(const StringVector &params);

        ConsoleCommandResult AddAssetStorage(const StringVector &params);

        ConsoleCommandResult ListAssetStorages(const StringVector &params);

        ConsoleCommandResult ConsoleRefreshHttpStorages(const StringVector &params);

        //! returns name of this module. Needed for logging.
        static const std::string &NameStatic() { return type_name_static_; }

    public slots:
        /// Loads from all the registered local storages all assets that have the given suffix.
        /// Type can also be optionally specified
        /// \todo Will be replaced with AssetStorage's GetAllAssetsRefs / GetAllAssets functionality
        void LoadAllLocalAssetsWithSuffix(const QString &suffix, const QString &assetType = "");
        
        /// Refreshes asset refs of all http storages
        void RefreshHttpStorages();
        
        /// If we are the server, this function gets called whenever a new connection is received. Populates the response data with the known asset storages in this server.
        void ServerNewUserConnected(int connectionID, UserConnection *connection, UserConnectedResponseData *responseData);
        /// If we are the client, this function gets called when we connect to a server. Adds all storages received from the server to our storage list.
        void ClientConnectedToServer(UserConnectedResponseData *responseData);
        /// If we are the client, this functio gets called when we disconnected. Removes all storages received from the server from our storage list.
        void ClientDisconnectedFromServer();

    private:
        void ProcessCommandLineOptions();

        //! Type name of the module.
        static std::string type_name_static_;

        /// When the client connects to the server, it adds to its list of known storages all the storages on the server side.
        /// To be able to also remove these storages from being used after we disconnect, we track all the server-originated storages here.
        std::vector<AssetStorageWeakPtr> storagesReceivedFromServer;
    };
}

#endif
