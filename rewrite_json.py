import json
import re

with open("config/app.jsonc", "r") as f:
    text = f.read()

text = re.sub(r'//.*', '', text)
data = json.loads(text)

scene = data["scene"]

new_world = []
for entity in scene.get("world", []):
    components = entity.get("components", [])
    is_cam = any(c.get("type", "") == "Camera" for c in components)
    if is_cam:
        new_world.append(entity)
        continue
        
    children = entity.get("children", [])
    is_cam_child = False
    for child in children:
        child_comps = child.get("components", [])
        if any(c.get("type", "") == "Camera" for c in child_comps):
            is_cam_child = True
    if is_cam_child or any(c.get("type", "") == "Free Camera Controller" for c in components):
        new_world.append(entity)
        continue
    
    if entity.get("name") == "City Model Test":
        entity["components"][0]["material"] = "city_mat"
        new_world.append(entity)

scene["world"] = new_world

# Delete wrongly placed materials
if "materials" in scene:
    del scene["materials"]

if "materials" not in scene["assets"]:
    scene["assets"]["materials"] = {}

scene["assets"]["materials"]["city_mat"] = {
    "type": "tinted",
    "shader": "tinted",
    "pipelineState": {
        "faceCulling": { "enabled": False },
        "depthTesting": { "enabled": True }
    },
    "tint": [1, 1, 1, 1]
}

data["start-scene"] = "menu"

with open("config/app.jsonc", "w") as f:
    json.dump(data, f, indent=4)
print("SUCCESS")
