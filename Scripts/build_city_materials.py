"""Generate reusable world-space architectural surfaces and soft collapse dust."""
import unreal

edit = unreal.MaterialEditingLibrary
assets = unreal.AssetToolsHelpers.get_asset_tools()

def material(name):
    path = '/Game/Materials/' + name
    result = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else assets.create_asset(name, '/Game/Materials', unreal.Material, unreal.MaterialFactoryNew())
    edit.delete_all_material_expressions(result)
    return result

def node(mat, kind, **properties):
    result = edit.create_material_expression(mat, kind)
    for key, value in properties.items():
        result.set_editor_property(key, value)
    return result

def link(source, target, pin):
    if not edit.connect_material_expressions(source, '', target, pin):
        raise RuntimeError(f'City material connection failed: {pin}')

city = material('M_City')
city.set_editor_property('used_with_instanced_static_meshes', True)
color = node(city, unreal.MaterialExpressionVectorParameter, parameter_name='Color', default_value=unreal.LinearColor(.3, .32, .33, 1))
position = node(city, unreal.MaterialExpressionWorldPosition)
normal = node(city, unreal.MaterialExpressionPixelNormalWS)
kind = node(city, unreal.MaterialExpressionScalarParameter, parameter_name='SurfaceType', default_value=0)
pattern = node(city, unreal.MaterialExpressionCustom, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for name in ['P', 'N', 'Tint', 'Kind']:
    item = unreal.CustomInput()
    item.set_editor_property('input_name', name)
    inputs.append(item)
pattern.set_editor_property('inputs', inputs)
pattern.set_editor_property('code', r'''
float2 uv = abs(N.z) > .65 ? P.xy : (abs(N.x) > .65 ? P.yz : P.xz);
float grain = frac(sin(dot(floor(uv / 3), float2(12.9898, 78.233))) * 43758.5453);
float stain = frac(sin(dot(floor(uv / float2(60, 1600)), float2(39.346, 11.135))) * 17453.17);
float2 tile = abs(frac(uv / float2(300, 320)) - .5);
float seam = smoothstep(.472, .493, max(tile.x, tile.y));
float worn = .90 + .10 * grain - .18 * stain;
float shade = Kind < .5 ? worn * (1 - seam * .18) :
              Kind < 1.5 ? .86 + .14 * stain :
              Kind < 2.5 ? .87 + .13 * grain - seam * .08 : .82 + .18 * grain;
return Tint.rgb * shade;
''')
for source, pin in [(position, 'P'), (normal, 'N'), (color, 'Tint'), (kind, 'Kind')]:
    link(source, pattern, pin)
edit.connect_material_property(pattern, '', unreal.MaterialProperty.MP_BASE_COLOR)
for parameter, value, prop in [('Roughness', .82, unreal.MaterialProperty.MP_ROUGHNESS), ('Metallic', .04, unreal.MaterialProperty.MP_METALLIC)]:
    scalar = node(city, unreal.MaterialExpressionScalarParameter, parameter_name=parameter, default_value=value)
    edit.connect_material_property(scalar, '', prop)
glow = node(city, unreal.MaterialExpressionScalarParameter, parameter_name='Glow', default_value=0)
emission = node(city, unreal.MaterialExpressionMultiply)
link(color, emission, 'A')
link(glow, emission, 'B')
edit.connect_material_property(emission, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
edit.recompile_material(city)
unreal.EditorAssetLibrary.save_loaded_asset(city)

dust = material('M_Dust')
dust.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
dust.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
dust.set_editor_property('two_sided', False)
tint = node(dust, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(.23, .20, .16, 1))
edit.connect_material_property(tint, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
edge = node(dust, unreal.MaterialExpressionFresnel, exponent=2.0, base_reflect_fraction=0.0)
inverse = node(dust, unreal.MaterialExpressionOneMinus)
link(edge, inverse, '')
opacity = node(dust, unreal.MaterialExpressionMultiply, const_b=.2)
link(inverse, opacity, 'A')
fade = node(dust, unreal.MaterialExpressionDepthFade, fade_distance_default=180)
link(opacity, fade, 'Opacity')
edit.connect_material_property(fade, '', unreal.MaterialProperty.MP_OPACITY)
edit.recompile_material(dust)
unreal.EditorAssetLibrary.save_loaded_asset(dust)
unreal.log('EVA_CITY_MATERIALS_OK')
