import unreal
from pathlib import Path
assets=unreal.AssetToolsHelpers.get_asset_tools()
for obj in (Path(__file__).parent/'GeneratedModels').glob('*.obj'):
    if '-EvaRamielOnly' in unreal.SystemLibrary.get_command_line() and obj.stem!='RamielCrystal': continue
    task=unreal.AssetImportTask()
    task.filename=str(obj);task.destination_path='/Game/Models';task.destination_name=obj.stem
    task.automated=True;task.replace_existing=True;task.save=True
    options=unreal.FbxImportUI()
    options.import_mesh=True;options.import_materials=False;options.import_textures=False
    options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.combine_meshes=True
    options.static_mesh_import_data.auto_generate_collision=False
    options.static_mesh_import_data.convert_scene=False
    task.options=options
    task.factory=unreal.FbxFactory()
    assets.import_asset_tasks([task])
    if not task.imported_object_paths: raise RuntimeError('Mesh import failed: '+str(obj))
    unreal.log('EVA_MODEL '+str(task.imported_object_paths))
unreal.log('EVA_MODELS_OK')
