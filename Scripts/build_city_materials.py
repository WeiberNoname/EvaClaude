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
# Instanced batches can carry their own colour (containers, cars, rubble); plain meshes read white.
variation = node(city, unreal.MaterialExpressionPerInstanceCustomData3Vector, data_index=0, const_default_value=unreal.LinearColor(1, 1, 1, 1))
tinted = node(city, unreal.MaterialExpressionMultiply)
link(color, tinted, 'A')
link(variation, tinted, 'B')
position = node(city, unreal.MaterialExpressionWorldPosition)
normal = node(city, unreal.MaterialExpressionPixelNormalWS)
kind = node(city, unreal.MaterialExpressionScalarParameter, parameter_name='SurfaceType', default_value=0)
time = node(city, unreal.MaterialExpressionTime)
pattern = node(city, unreal.MaterialExpressionCustom, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for name in ['P', 'N', 'Tint', 'Kind', 'Time']:
    item = unreal.CustomInput()
    item.set_editor_property('input_name', name)
    inputs.append(item)
pattern.set_editor_property('inputs', inputs)
# Surface types: 0 concrete, 1 glass, 2 metal, 3 asphalt, 4 water, 5 corrugated metal, 6 foliage,
# 7 scorched concrete, 8 painted height bands, 9 hazard stripes, 10 flickering fire.
pattern.set_editor_property('code', r'''
float2 uv = abs(N.z) > .65 ? P.xy : (abs(N.x) > .65 ? P.yz : P.xz);
float grain = frac(sin(dot(floor(uv / 3), float2(12.9898, 78.233))) * 43758.5453);
float stain = frac(sin(dot(floor(uv / float2(60, 1600)), float2(39.346, 11.135))) * 17453.17);
float2 tile = abs(frac(uv / float2(300, 320)) - .5);
float seam = smoothstep(.472, .493, max(tile.x, tile.y));
float worn = .90 + .10 * grain - .18 * stain;
float grime = abs(N.z) < .65 ? lerp(.62, 1, saturate(P.z / 260)) : 1;
float3 c = Tint.rgb;
if (Kind < .5) return c * worn * (1 - seam * .18) * grime;
if (Kind < 1.5) return c * (.86 + .14 * stain);
if (Kind < 2.5) return c * (.87 + .13 * grain - seam * .08) * grime;
if (Kind < 3.5) return c * (.82 + .18 * grain);
if (Kind < 4.5)
{
    float swell = sin(P.x * .0021 + Time * .8) * .5 + sin(P.y * .0034 - Time * .6 + P.x * .0007) * .5;
    float chop = sin(P.x * .013 + P.y * .009 + Time * 2.3) * sin(P.y * .017 - Time * 1.7);
    float glint = step(.992, frac(sin(dot(floor(P.xy / 70 + floor(Time * 2)), float2(12.9898, 78.233))) * 43758.5453));
    return c * (.78 + .16 * swell + .08 * chop) + glint * .35;
}
if (Kind < 5.5)
{
    float along = abs(N.x) > .65 ? P.y : P.x;
    float rib = abs(frac(along / 24) - .5) * 2;
    float rust = frac(sin(dot(floor(uv / float2(160, 700)), float2(17.13, 41.7))) * 9731.1);
    float streak = step(.72, rust) * saturate(1 - frac(P.z / 700) * 1.4);
    return c * (.74 + .26 * rib) * (1 - .25 * streak) * grime;
}
if (Kind < 6.5)
{
    float clump = frac(sin(dot(floor(P.xy / 45 + P.z / 70), float2(27.1, 61.7))) * 5453.3);
    float light = saturate(N.z * .5 + .6);
    return c * (.55 + .45 * clump) * light;
}
if (Kind < 7.5)
{
    float blot = frac(sin(dot(floor(uv / 150), float2(7.13, 19.7))) * 2713.1);
    float streak = frac(sin(floor(uv.x / 85) * 91.7) * 1731.1);
    return c * worn * lerp(.16, .6, blot * streak) * (1 - seam * .3);
}
if (Kind < 8.5) return (frac(P.z / 700) < .5 ? c : float3(.8, .8, .76)) * worn;
if (Kind < 9.5) return frac((P.x + P.y + P.z) / 260) < .5 ? c : float3(.025, .025, .022);
float flicker = .62 + .38 * sin(Time * 17 + P.x * .011) * sin(Time * 11.3 + P.y * .013 + P.z * .007);
return c * flicker;
''')
for source, pin in [(position, 'P'), (normal, 'N'), (tinted, 'Tint'), (kind, 'Kind'), (time, 'Time')]:
    link(source, pattern, pin)
edit.connect_material_property(pattern, '', unreal.MaterialProperty.MP_BASE_COLOR)
for parameter, value, prop in [('Roughness', .82, unreal.MaterialProperty.MP_ROUGHNESS), ('Metallic', .04, unreal.MaterialProperty.MP_METALLIC)]:
    scalar = node(city, unreal.MaterialExpressionScalarParameter, parameter_name=parameter, default_value=value)
    edit.connect_material_property(scalar, '', prop)
glow = node(city, unreal.MaterialExpressionScalarParameter, parameter_name='Glow', default_value=0)
emission = node(city, unreal.MaterialExpressionMultiply)
# Emission follows the patterned colour so fire flickers and painted signs keep their bands.
link(pattern, emission, 'A')
link(glow, emission, 'B')
edit.connect_material_property(emission, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
edit.recompile_material(city)
unreal.EditorAssetLibrary.save_loaded_asset(city)

dust = material('M_Dust')
dust.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
dust.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
dust.set_editor_property('two_sided', False)
# Pooled dust and smoke are instanced; per-instance data fades each puff (0) and darkens smoke (1).
dust.set_editor_property('used_with_instanced_static_meshes', True)
tint = node(dust, unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(.23, .20, .16, 1))
alpha = node(dust, unreal.MaterialExpressionPerInstanceCustomData, data_index=0, const_default_value=1.0)
bright = node(dust, unreal.MaterialExpressionPerInstanceCustomData, data_index=1, const_default_value=1.0)
shade = node(dust, unreal.MaterialExpressionMultiply)
link(tint, shade, 'A')
link(bright, shade, 'B')
edit.connect_material_property(shade, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
edge = node(dust, unreal.MaterialExpressionFresnel, exponent=2.0, base_reflect_fraction=0.0)
inverse = node(dust, unreal.MaterialExpressionOneMinus)
link(edge, inverse, '')
opacity = node(dust, unreal.MaterialExpressionMultiply, const_b=.2)
link(inverse, opacity, 'A')
faded = node(dust, unreal.MaterialExpressionMultiply)
link(opacity, faded, 'A')
link(alpha, faded, 'B')
fade = node(dust, unreal.MaterialExpressionDepthFade, fade_distance_default=180)
link(faded, fade, 'Opacity')
edit.connect_material_property(fade, '', unreal.MaterialProperty.MP_OPACITY)
edit.recompile_material(dust)
unreal.EditorAssetLibrary.save_loaded_asset(dust)
unreal.log('EVA_CITY_MATERIALS_OK')
