if (!framework.IsHeadless())
{
    engine.ImportExtension("qt.core");
    engine.ImportExtension("qt.gui");

    var sceneAction = null;
    var assetAction = null;
    
    var mainwin = ui.MainWindow();

    // File
    var fileMenu = mainwin.AddMenu("&File", 100);
    
    // Load and save scene
    var importWebAction = mainwin.AddMenuAction("&File", "Import Web Scene", new QIcon("./data/ui/images/folder_closed.png"));
    importWebAction.triggered.connect(OpenWebScene);
    var exportAction = mainwin.AddMenuAction("&File", "Save as...", new QIcon("./data/ui/images/resource.png"));
    exportAction.triggered.connect(SaveScene);
    //fileMenu.addSeparator();
    
    if (framework.GetModuleQObj("UpdateModule"))
    	mainwin.AddMenuAction("&File", "Check Updates", new QIcon("./data/ui/images/icon/update.ico")).triggered.connect(CheckForUpdates);
       
    // Reconnect menu items for client only
    if (!server.IsAboutToStart())
    {
        var disconnectAction = mainwin.AddMenuAction("&File", "Disconnect", new QIcon("./data/ui/images/icon/disconnect.ico"));
        disconnectAction.triggered.connect(Disconnect);
        client.Connected.connect(Connected);
        client.Disconnected.connect(Disconnected);
        Disconnected();
    }
    mainwin.AddMenuAction("&File", "Quit", new QIcon("./data/ui/images/icon/system-shutdown.ico")).triggered.connect(Quit);

    // View
    var viewMenu = mainwin.AddMenu("&View", 90);
    if (framework.GetModuleQObj("SceneStructure"))
    {
        assetAction = mainwin.AddMenuAction("&View", "Assets", new QIcon("./data/ui/images/fileIcons.png"));
        assetAction.triggered.connect(OpenAssetsWindow);
        sceneAction = mainwin.AddMenuAction("&View", "Scene", new QIcon("./data/ui/images/fileList.png"));
        sceneAction.triggered.connect(OpenSceneWindow);
    }

    if (framework.GetModuleQObj("Console"))
    	mainwin.AddMenuAction("&View", "Console").triggered.connect(OpenConsoleWindow);

    if (framework.GetModuleQObj("DebugStats"))
    	mainwin.AddMenuAction("&View", "Profiler").triggered.connect(OpenProfilerWindow);

    if (framework.GetModuleQObj("Environment"))
    {
    	mainwin.AddMenuAction("&View", "Terrain Editor").triggered.connect(OpenTerrainEditor);
    	mainwin.AddMenuAction("&View", "Post-processing").triggered.connect(OpenPostProcessWindow);
    }

    if (framework.GetModuleQObj("PythonScript"))
    	mainwin.AddMenuAction("&View", "Python Console").triggered.connect(OpenPythonConsole);
        
    // Settings
    mainwin.AddMenu("&Settings", 80);
    if (framework.GetModuleQObj("CAVEStereo"))
    {
        var caveSettings = mainwin.AddMenuAction("&Settings", "Cave");
        caveSettings.triggered.connect(OpenCaveWindow);
        var stereoSettings = mainwin.AddMenuAction("&Settings", "Stereoscopy");
        stereoSettings.triggered.connect(OpenStereoscopyWindow);
    }
    
    // Help
    var helpMenu = mainwin.AddMenu("&Help", 10);
    mainwin.AddMenuAction("&Help", "Wiki", new QIcon("./data/ui/images/icon/browser.ico")).triggered.connect(OpenWikiUrl);
    mainwin.AddMenuAction("&Help", "Doxygen", new QIcon("./data/ui/images/icon/browser.ico")).triggered.connect(OpenDoxygenUrl);
    mainwin.AddMenuAction("&Help", "Mailing list", new QIcon("./data/ui/images/icon/browser.ico")).triggered.connect(OpenMailingListUrl);
    
    function NewScene() {
        scene.RemoveAllEntities();
    }

    function Reconnect() {
        client.Reconnect();
    }

    function Disconnect() {
        client.Logout();
    }

    function Connected() {
        disconnectAction.setEnabled(true);
        importWebAction.setEnabled(true);
        exportAction.setEnabled(true);
        ui.EmitAddAction(sceneAction);
        ui.EmitAddAction(assetAction);
    }

    function Disconnected() {
        disconnectAction.setEnabled(false);
        importWebAction.setEnabled(false);
        exportAction.setEnabled(false);
    }

    function Quit() {
        framework.Exit();
    }

    function CheckForUpdates() {
        if (framework.GetModuleQObj("UpdateModule"))
            framework.GetModuleQObj("UpdateModule").RunUpdater("/checknow");
    }

    function OpenMailingListUrl() {
        if (server.IsRunning())
            QDesktopServices.openUrl(new QUrl("http://groups.google.com/group/realxtend/"));
        else
            ui.EmitOpenUrl(new QUrl("http://groups.google.com/group/realxtend/"));
    }
    
    function OpenWikiUrl() {
        if (server.IsRunning())
            QDesktopServices.openUrl(new QUrl("http://wiki.realxtend.org/"));
        else
            ui.EmitOpenUrl(new QUrl("http://wiki.realxtend.org/"));
    }

    function OpenDoxygenUrl() {
        if (server.IsRunning())
            QDesktopServices.openUrl(new QUrl("http://www.realxtend.org/doxygen/"));
        else
            ui.EmitOpenUrl(new QUrl("http://www.realxtend.org/doxygen/"));
    }

    function OpenSceneWindow() {
        framework.GetModuleQObj("SceneStructure").ToggleSceneStructureWindow();
    }

    function OpenAssetsWindow() {
        framework.GetModuleQObj("SceneStructure").ToggleAssetsWindow();
    }

    function OpenProfilerWindow() {
        console.ExecuteCommand("prof");
    }

    function OpenTerrainEditor() {
        framework.GetModuleQObj("Environment").ShowTerrainWeightEditor();
    }

    function OpenPostProcessWindow() {
        framework.GetModuleQObj("Environment").ShowPostProcessWindow();
    }

    function OpenPythonConsole() {
        console.ExecuteCommand("pythonconsole");
    }

    function OpenConsoleWindow() {
        framework.GetModuleQObj("Console").ToggleConsole();
    }

    function OpenStereoscopyWindow() {
        framework.GetModuleQObj("CAVEStereo").ShowStereoscopyWindow();
    }

    function OpenCaveWindow() {
        framework.GetModuleQObj("CAVEStereo").ShowCaveWindow();
    }
    
    function OpenLocalScene() {
        var currentScene = framework.Scene().GetDefaultSceneRaw();
        if (currentScene == null)
            return;
        
        var filename = QFileDialog.getOpenFileName(ui.MainWindow(), "Import Scene", QDir.currentPath() + "/scenes", "Tundra Scene (*.txml *.tbin)");
        if (filename == null || filename == "")
            return;
        if (!QFile.exists(filename))
            return;
            
        var fileninfo = new QFileInfo(filename);
        if (fileninfo.suffix() == "txml")
            currentScene.LoadSceneXML(filename, false, false, 3);
        else if (fileninfo.suffix() == "tbin")
            currentScene.LoadSceneBinary(filename, false, false, 3);
    }
    
    function OpenWebScene() {
        var webRef = QInputDialog.getText(ui.MainWindow(), "Import Web Scene", "Insert a txml or tbin scene url", QLineEdit.Normal, "http://", Qt.Dialog);
        if (webRef == null || webRef == "")
            return;
        var ext = webRef.substring(webRef.length-4);
        var qUrl = QUrl.fromUserInput(webRef);
        if (!qUrl.isValid())
            return;
        
        if (ext != "txml" && ext != "tbin")
            return;
        var transfer = asset.RequestAsset(qUrl.toString()).get();
        transfer.Loaded.connect(WebSceneLoaded);
    }
    
    function WebSceneLoaded(assetptr) {
        var currentScene = framework.Scene().GetDefaultSceneRaw();
        if (currentScene == null)
            return;
                   
        var asset = assetptr.get();
        var diskSource = asset.DiskSource();
        var fileninfo = new QFileInfo(diskSource);
        if (fileninfo.suffix() == "txml")
            currentScene.LoadSceneXML(diskSource, false, false, 3);
        else if (fileninfo.suffix() == "tbin")
            currentScene.LoadSceneBinary(diskSource, false, false, 3);
    }
    
    function SaveScene() {
        var currentScene = framework.Scene().GetDefaultSceneRaw();
        if (currentScene == null)
            return;
            
        var filename = QFileDialog.getSaveFileName(ui.MainWindow(), "Export Scene", QDir.currentPath() + "/scenes", "Tundra Scene (*.txml *.tbin)");
        if (filename == null || filename == "")
            return;
        var ext = new QFileInfo(filename).suffix();
        if (ext != "txml" && ext != "tbin")
        {
            QMessageBox.information(ui.MainWindow(), "Invalid extension", "Invalid Tundra scene file extension '" + ext + "'");
            return;
        }
           
        if (ext == "txml")
            currentScene.SaveSceneXML(filename);
        else if (ext == "tbin")
            currentScene.SaveSceneBinary(filename);
    }
}
