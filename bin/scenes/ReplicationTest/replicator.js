// Scene replication stresstest

var isServer = server.IsRunning();

if (isServer)
{
    print("Replication stresstest, client");
    frame.Updated.connect(PerformOperation);
}
else
{
    print("Replication stresstest, server");
    frame.Updated.connect(PerformOperation);
}

function PerformOperation()
{
    var operation = Math.floor(Math.random() * 20);
    
    if (operation == 1)
    {
        var entities = scene.GetAllEntities();
        if (entities.length >= 50)
            return;
        // Create entity
        var newEnt = scene.CreateEntity(0, ["EC_Placeable", "EC_RigidBody", "EC_Mesh"]);
        var transform = newEnt.placeable.transform;
        transform.pos = new float3(Math.random() * 80 - 40, 10, Math.random() * 80 - 40);
        transform.rot = new float3(Math.random() * 360, Math.random() * 360, Math.random() * 360);
        transform.scale = new float3(Math.random() + 1, Math.random() + 1, Math.random() + 1);
        newEnt.placeable.transform = transform;
        var meshRef = newEnt.mesh.meshRef;
        if (isServer)
            meshRef.ref = "fish.mesh";
        else
            meshRef.ref = "WoodPallet.mesh";
        newEnt.mesh.meshRef = meshRef;
        newEnt.rigidbody.mass = 5;
        newEnt.rigidbody.shapeType = 6; // Convex hull
        newEnt.name = "Test";
        print("Created new entity " + newEnt.id);
    }
    if (operation == 2)
    {
        // Delete entity
        var entities = scene.GetAllEntities();
        var index = Math.floor(Math.random() * (entities.length - 1));
        var delEnt = entities[index];
        if (delEnt.name == "Test")
        {
            print("Removing entity " + delEnt.id);
            scene.RemoveEntity(delEnt.id);
        }
    }
    if (operation == 3)
    {
        // Remove the RigidBody component from the entity
        var entities = scene.GetAllEntities();
        var index = Math.floor(Math.random() * (entities.length - 1));
        var delEnt = entities[index];
        if (delEnt.name == "Test")
        {
            print("Removing rigidbody from entity " + delEnt.id);
            delEnt.RemoveComponent("EC_RigidBody");
        }
    }
}


