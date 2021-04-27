"""
BlenderVisual.blender_element

Elementary components and operations

Created on Tue Jun 25 11:39:49 2019
@author: Chang Gong
"""

import bpy
import numpy as np
import math

#%%

##############################################################################
## LIGHTING (three point)
##############################################################################

# general light source creation
def createLampObj(name,light, strength, color, scene):
    lampObj = bpy.data.objects.new(name, light)
    scene.collection.objects.link(lampObj)
    light.use_nodes = True
    nodes = light.node_tree.nodes
    nodes['Emission'].inputs['Strength'].default_value = strength
    nodes['Emission'].inputs['Color'].default_value = color
    return lampObj

# create sunlight light source
def addSunLight(name, strength, color, resolution, scene):
    lamp = bpy.data.lights.new('lampSun', 'SUN')
    lampObj = createLampObj(name,lamp, strength, color, scene)
    lamp.shadow_soft_size = 0.01 # for sun
    return lampObj

# create Area light source
def addAreaLight(name, strength, color, size, scene):
    lamp = bpy.data.lights.new('lampArea', 'AREA')
    lampObj = createLampObj(name,lamp, strength, color, scene)
    lamp.size = size
    return lampObj

# create point light source
def addPointLight(name, strength, color, shadowsoft, scene):
    lamp = bpy.data.lights.new('lampPoint', 'POINT')
    lampObj = createLampObj(name,lamp, strength, color, scene)
    lamp.shadow_soft_size = shadowsoft
    return lampObj

# create spot light source
def addSpotLight(name, strength, color, size, shadowsoft, angle, blend, scene):
    lamp = bpy.data.lights.new('lampSpot', 'SPOT')
    lampObj = createLampObj(name,lamp, strength, color, scene)
    lampObj.scale *= size
    lamp.shadow_soft_size = shadowsoft
    lamp.spot_size = angle
    lamp.spot_blend = blend
    return lampObj

########################################
## Add objects
########################################    
    
def create_Vertices (name, scene, verts):
    # Create mesh and object
    me = bpy.data.meshes.new(name+'Mesh')
    ob = bpy.data.objects.new(name, me)
    #ob.show_name = True
    # Link object to scene
    scene.collection.objects.link(ob)
    me.from_pydata(list(verts), [], [])
    # Update mesh with new data
    # me.update()
    return ob

def create_Edges (name, scene, verts, edges):
    me = bpy.data.meshes.new(name+'_mesh')
    ob = bpy.data.objects.new(name, me)
    scene.collection.objects.link(ob)
    me.from_pydata(list(verts), list(edges), [])
    me.update()
    return ob

def create_Faces (name, scene, verts, simplices):
    me = bpy.data.meshes.new(name+'_mesh')
    ob = bpy.data.objects.new(name, me)
    scene.collection.objects.link(ob)
    # if faces and (not edges), numpy array cause error.
    me.from_pydata(list(verts), [], [vv for vv in simplices])
    me.update()
    return ob

def swap_face(ob, verts, simplices):
    me_old = ob.data
    me = bpy.data.meshes.new(ob.name+'_mesh')
    me.from_pydata(list(verts), [], [vv for vv in simplices])
    ob.data = me
    bpy.data.meshes.remove(me_old)
    #me.update()
    return ob

def visual_box(name, loc, dim, thickness, color):
    bpy.ops.mesh.primitive_cube_add(location=loc)
    box = bpy.context.object
    box.dimensions=dim
    box.modifiers.new('wirebox', type='WIREFRAME')
    box.modifiers['wirebox'].thickness=thickness
    box.name = name
    box.cycles_visibility.shadow = False
    box.cycles_visibility.diffuse = False
    box.cycles_visibility.transmission = False

    mat=createMaterial(name='virtual_line')
    nodes = mat.node_tree.nodes
    nodes.new(type='ShaderNodeEmission')
    nodes['Emission'].inputs['Color'].default_value = color
    inp = nodes['Material Output'].inputs['Surface']
    outp = nodes['Emission'].outputs['Emission']
    mat.node_tree.links.new(inp,outp)
    box.active_material = mat
    return box

## check dim and imgMat picture have the same aspect ratio
def createImagePlane(name, dim, imgMat):
    bpy.ops.mesh.primitive_plane_add()
    plane = bpy.context.object
    plane.name = name
    plane.dimensions = dim
    plane.active_material = imgMat
    #me = plane.data
    #me.uv_textures.new('UVmap')
    return plane

# apply smooth shading to mesh
def smooth_shading(obj):
    for pol in obj.data.polygons:
        pol.use_smooth = True
########################################
## Add objects - network related
########################################    
# new curve consisting multiple splines
# inputs: 
# name: name of curve
# v: vertices
# vertice_list: list[list[vertices]]. inner list contains vertex indices
def creaetCurveFromList(name, v, vertice_list):
    curve_data = bpy.data.curves.new(name=name, type='CURVE')
    curve_data.dimensions = '3D'
    for i, verts in enumerate(vertice_list):
        curve_data.splines.new(type = 'POLY')
        points = curve_data.splines[i].points
        nrPoints = len(verts)
        points.add(nrPoints-1)
        for j, p in enumerate(verts):
            points[j].co = np.append(v[p,:],0)
    return curve_data
    

# return edge value by taking average of vertex values of two end points
def vertToEdgeAverage(e, v, v_value):
    e_value = [(v_value[ev[0]]+v_value[ev[1]])/2 for ev in e]
    return np.asarray(e_value)

#####################
# material
#####################
# create empty material
# when case diffuse shader is not needed
def createMaterial(name):
    mat=bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes=mat.node_tree.nodes
    node=nodes['Principled BSDF']
    nodes.remove(node)
    return mat

# color -> diffuse shader.
def createDiffussionSurfaceMaterial(name, color):
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes    
    nodes.new(type='ShaderNodeBsdfDiffuse')
    nodes['Diffuse BSDF'].inputs['Color'].default_value = color
    inp = nodes['Material Output'].inputs['Surface']
    outp = nodes['Diffuse BSDF'].outputs['BSDF']
    mat.node_tree.links.new(inp,outp)
    return mat

# color -> emission shader. 
# for box etc.
def createEmissionMaterial(name, color):
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes    
    nodes.new(type='ShaderNodeEmission')
    nodes['Emission'].inputs['Color'].default_value = color
    inp = nodes['Material Output'].inputs['Surface']
    outp = nodes['Emission'].outputs['Emission']
    mat.node_tree.links.new(inp,outp)
    return mat

# color -> transparent mix shader. 
#for isosurface etc.
def createTransparentMaterial(name, color):
    mat = createMaterial(name)
    nodes = mat.node_tree.nodes
    mix = nodes.new(type='ShaderNodeMixShader')
    mix.name = 'mix'
    trans = nodes.new(type='ShaderNodeBsdfTransparent')
    trans.name = 'trans'
    gloss = nodes.new(type='ShaderNodeBsdfGlossy')
    gloss.name = 'gloss'
    mat.node_tree.links.new(gloss.outputs['BSDF'], mix.inputs[1])
    mat.node_tree.links.new(trans.outputs['BSDF'], mix.inputs[2])
    mat.node_tree.links.new(mix.outputs[0], nodes['Material Output'].inputs['Surface'])
    gloss.inputs['Roughness'].default_value = 0.3
    mix.inputs['Fac'].default_value = 0.875
    trans.inputs['Color'].default_value = color
    return mat

# diffuse + glossy shader     
def createGlossyMaterial(name, color):
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.new(type='ShaderNodeBsdfDiffuse')
    mix = nodes.new(type='ShaderNodeMixShader')
    mix.name = 'mix'
    diff = nodes['Diffuse BSDF']
    diff.inputs['Color'].default_value = color
    gloss = nodes.new(type='ShaderNodeBsdfGlossy')
    gloss.name = 'gloss'
    links.new(gloss.outputs['BSDF'], mix.inputs[1])
    links.new(diff.outputs['BSDF'], mix.inputs[2])
    links.new(mix.outputs[0], nodes['Material Output'].inputs['Surface'])
    gloss.inputs['Roughness'].default_value = 0.3
    mix.inputs['Fac'].default_value = 0.875
    return mat

# plane uv -> image -> emission shader (other side white) 
def createPlaneTextureFromImage(name, imgTexture, pixelate = False):
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    mix = nodes.new(type='ShaderNodeMixShader')
    links.new(nodes['Material Output'].inputs['Surface'], mix.outputs[0])
    geo = nodes.new(type='ShaderNodeNewGeometry')
    links.new(geo.outputs['Backfacing'], mix.inputs['Fac'])
    back = nodes.new(type='ShaderNodeBsdfDiffuse')
    links.new(back.outputs['BSDF'], mix.inputs[2])
    #front = nodes.new(type='ShaderNodeBsdfDiffuse')
    #links.new(front.outputs['BSDF'], mix.inputs[1])
    front = nodes.new(type='ShaderNodeEmission')
    links.new(front.outputs['Emission'], mix.inputs[1])
    img = nodes.new(type='ShaderNodeTexImage')
    links.new(img.outputs['Color'], front.inputs['Color'])
    img.image = imgTexture
    img.extension = 'CLIP'
    if pixelate: # no interpolation
        img.interpolation = 'Closest'
    else:
        img.interpolation = 'Smart'
    #mapping = nodes.new(type='ShaderNodeMapping')
    #links.new(mapping.outputs['Vector'], img.inputs['Vector'])
    coord = nodes.new(type='ShaderNodeTexCoord')
    #links.new(coord.outputs['UV'], mapping.inputs['Vector'])    
    links.new(coord.outputs['UV'], img.inputs['Vector'])
    return mat

# plane uv -> image -> alpha/color 
def createTransparentTextureFromImage(name, imgTexture, pixelate = False):
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    mix = nodes.new(type='ShaderNodeMixShader')
    links.new(nodes['Material Output'].inputs['Surface'], mix.outputs[0])
    alpha = nodes.new(type='ShaderNodeBsdfTransparent')
    links.new(alpha.outputs['BSDF'], mix.inputs[1])
    color = nodes.new(type='ShaderNodeEmission')
    color.inputs[1].default_value = 1.5
    links.new(color.outputs['Emission'], mix.inputs[2])
    img = nodes.new(type='ShaderNodeTexImage')
    links.new(img.outputs['Color'], color.inputs['Color'])
    links.new(img.outputs['Alpha'], mix.inputs['Fac'])
    img.image = imgTexture
    img.extension = 'CLIP'
    if pixelate: # no interpolation
        img.interpolation = 'Closest'
    else:
        img.interpolation = 'Smart'
    #mapping = nodes.new(type='ShaderNodeMapping')
    #links.new(mapping.outputs['Vector'], img.inputs['Vector'])
    coord = nodes.new(type='ShaderNodeTexCoord')
    #links.new(coord.outputs['UV'], mapping.inputs['Vector'])    
    links.new(coord.outputs['UV'], img.inputs['Vector'])
    return mat

# vcol_name is the name for vertex_color        
def createVertexColorMaterial(name, vcol_name):
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.new(type='ShaderNodeBsdfDiffuse')
    attrib = nodes.new(type='ShaderNodeAttribute')
    attrib.attribute_name = vcol_name
    mix = nodes.new(type='ShaderNodeMixShader')
    mix.name = 'mix'
    diff = nodes['Diffuse BSDF']
    gloss = nodes.new(type='ShaderNodeBsdfGlossy')
    gloss.name = 'gloss'
    links.new(gloss.outputs['BSDF'], mix.inputs[1])
    links.new(diff.outputs['BSDF'], mix.inputs[2])
    links.new(mix.outputs[0], nodes['Material Output'].inputs['Surface'])
    gloss.inputs['Roughness'].default_value = 0.5
    mix.inputs['Fac'].default_value = 0.875
    links.new(nodes['Diffuse BSDF'].inputs['Color'],attrib.outputs['Color'])
    return mat

# glowing transparent surface
# depth: transparent depth; weight:transparent Fresnel weight
def createXRayMaterial(name, color=(0,0.5,1,1), strength = 1, depth=1, weight=0.05):
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    mix0 = nodes.new(type='ShaderNodeMixShader')
    mix0.name = 'mix0'
    links.new(nodes['Material Output'].inputs['Surface'], mix0.outputs[0])
    mix1 = nodes.new(type='ShaderNodeMixShader')
    mix1.name = 'mix1'
    maxdepth = nodes.new(type='ShaderNodeMath')
    maxdepth.inputs[1].default_value = depth
    alpha = nodes.new(type='ShaderNodeBsdfTransparent')
    emission = nodes.new(type='ShaderNodeEmission')
    emission.inputs['Color'].default_value = color
    emission.inputs['Strength'].default_value = strength
    weight = nodes.new(type='ShaderNodeLayerWeight')
    weight.inputs['Blend'].default_value = 0.2
    path = nodes.new(type='ShaderNodeLightPath')
    links.new(path.outputs['Transparent Depth'], maxdepth.inputs[0])
    links.new(maxdepth.outputs[0], mix0.inputs['Fac'])
    links.new(weight.outputs['Fresnel'], mix1.inputs['Fac'])
    links.new(alpha.outputs[0], mix1.inputs[1])
    links.new(emission.outputs[0], mix1.inputs[2])
    links.new(alpha.outputs[0], mix0.inputs[1])
    links.new(mix1.outputs[0], mix0.inputs[2])
    return mat
#####################
# material: particles
#####################
## map color to particle system  by creating a 1-to-1
# pixel array. 
# particle id -> image -> diffuse shader    
def createColorMappedParticleMaterial(colors, name = 'particleMat'):
    rgba = np.expand_dims(colors, axis=1)
    image = createImage(name+'img_part_map_color', rgba)
    nrParticle = colors.shape[0]
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.new(type='ShaderNodeBsdfDiffuse')
    nPart = nodes.new(type='ShaderNodeParticleInfo')    
    nPlus = nodes.new(type='ShaderNodeMath')
    nPlus.operation = 'ADD'
    nPlus.inputs[1].default_value = .5
    mat.node_tree.links.new(nPlus.inputs[0],nPart.outputs['Index'])
    nDiv = nodes.new(type='ShaderNodeMath')
    nDiv.operation = 'DIVIDE'
    nDiv.name = 'div'
    nDiv.inputs[1].default_value = nrParticle
    mat.node_tree.links.new(nDiv.inputs[0],nPlus.outputs[0])
    nComb = nodes.new(type='ShaderNodeCombineXYZ')
    nComb.inputs[1].default_value = 0.5
    mat.node_tree.links.new(nComb.inputs[0],nDiv.outputs[0])
    nImg = nodes.new(type='ShaderNodeTexImage')
    nImg.interpolation = 'Closest'
    nImg.extension = 'REPEAT'
    nImg.image = image
    links.new(nImg.inputs[0],nComb.outputs[0])
    links.new(nodes['Diffuse BSDF'].inputs['Color'],nImg.outputs[0])
    links.new(nodes['Material Output'].inputs['Surface'],nodes['Diffuse BSDF'].outputs['BSDF'])
    return mat


def updateParticleColor(mat, colors):
    rgba = np.expand_dims(colors,axis=1)
    nodes = mat.node_tree.nodes
    image = nodes['Image Texture'].image
    image_name = image.name
    bpy.data.images.remove(image, do_unlink = True)
    image_new = createImage(image_name, rgba)
    nodes['Image Texture'].image = image_new
    nrColor = len(colors)
    nDiv = mat.node_tree.nodes['div']
    nDiv.inputs[1].default_value = nrColor
    
## map color to particle system  by 
#  creating a color ramp factored by particle lifetime
#  lifetime needs to be manually updated every time tick
#  or it will be overwritten by particle system
#  colormap: n x 4, rgba of color stops
def createColorRampParticleMaterial(colormap, attrib = 'lifetime', name = 'particleRamp'):
    nrColor = colormap.shape[0]
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.new(type='ShaderNodeBsdfDiffuse')
    nPart = nodes.new(type='ShaderNodeParticleInfo')    
    cRamp = nodes.new(type='ShaderNodeValToRGB')
    cr = cRamp.color_ramp
    cr.interpolation = 'LINEAR'
    for i in range(1, nrColor-1):
        cr.elements.new(i/(nrColor-1))
        cr.elements[i].color = colormap[i]
    cr.elements[0].color = colormap[0]
    cr.elements[-1].color = colormap[-1]
    if attrib == 'lifetime':
        links.new(nPart.outputs['Lifetime'], cRamp.inputs['Fac'])
    elif attrib == 'velocity':
        nSep = nodes.new(type='ShaderNodeSeparateXYZ')
        links.new(nSep.outputs[0], cRamp.inputs['Fac'])
        links.new(nPart.outputs['Velocity'], nSep.inputs[0])
    else:
        raise ValueError('unsupported attribute: '.format(attrib))
    links.new(nodes['Diffuse BSDF'].inputs['Color'],cRamp.outputs['Color'])
    links.new(nodes['Material Output'].inputs['Surface'],nodes['Diffuse BSDF'].outputs['BSDF'])
    return mat

## map color to particle system  by 
#  creating a color ramp factored by OSL script
#  values: n x 1, factor
#  colormap: n x 4, rgba of color stops
def createScriptColorRampParticleMaterial(values, colormap, name = 'particleRamp'):
    nrColor = colormap.shape[0]
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.new(type='ShaderNodeBsdfDiffuse')
    t = create_OSL_script('osl_color', values, digits=2)
    nPart = nodes.new(type='ShaderNodeParticleInfo')
    nScript = mat.node_tree.nodes.new('ShaderNodeScript')
    nScript.mode = 'INTERNAL'
    nScript.script = t
    links.new(nPart.outputs['Index'], nScript.inputs['id'])    
    cRamp = nodes.new(type='ShaderNodeValToRGB')
    cr = cRamp.color_ramp
    cr.interpolation = 'LINEAR'
    for i in range(1, nrColor-1):
        cr.elements.new(i/(nrColor-1))
        cr.elements[i].color = colormap[i]
    cr.elements[0].color = colormap[0]
    cr.elements[-1].color = colormap[-1]
    links.new(nScript.outputs['Val'], cRamp.inputs['Fac'])
    links.new(nodes['Diffuse BSDF'].inputs['Color'],cRamp.outputs['Color'])
    links.new(nodes['Material Output'].inputs['Surface'],nodes['Diffuse BSDF'].outputs['BSDF'])
    return mat

## swap osl node script
def updateMaterialScript(mat, values):
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nScript = nodes.get('Script')
    cRamp = nodes.get('ColorRamp')
    nPart = nodes.get('Particle Info')
    t = create_OSL_script('osl_color', values, digits=2)
    nScript.script = t
    links.new(nPart.outputs['Index'], nScript.inputs['id'])
    links.new(nScript.outputs['Val'], cRamp.inputs['Fac'])
    return
    
#####################
# material: edge
#####################
# uv -> 1 pixel row image -> diffuse shader
# internal image.
def createColorMappedEdgeMaterial(colors, name = 'splineColorMat'):
    rgba = np.expand_dims(colors,axis=1)
    imageName = name+'_img_edge_map_color'
    image = createImage(imageName, rgba)
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.new(type='ShaderNodeBsdfDiffuse')
    nImg = nodes.new(type='ShaderNodeTexImage')
    nImg.interpolation = 'Closest'
    nImg.extension = 'CLIP'
    nImg.image = image
    mat.node_tree.links.new(nodes['Diffuse BSDF'].inputs['Color'],nImg.outputs[0])
    mat.node_tree.links.new(nodes['Material Output'].inputs['Surface'],nodes['Diffuse BSDF'].outputs['BSDF'])
    return mat

# uv -> 1 pixel row image -> diffuse shader
# images are saved to file.
def createColorMappedEdgeMaterialExternalImage(colors, imageDir, name = 'splineColorMat'):
    rgba = np.expand_dims(colors,axis=1)
    imageName = name+'_img_edge_map_color'
    image = createExternalImage(imageName, imageDir, rgba)
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.new(type='ShaderNodeBsdfDiffuse')
    nImg = nodes.new(type='ShaderNodeTexImage')
    nImg.interpolation = 'Closest'
    nImg.extension = 'CLIP'
    nImg.image = image
    mat.node_tree.links.new(nodes['Diffuse BSDF'].inputs['Color'],nImg.outputs[0])
    mat.node_tree.links.new(nodes['Material Output'].inputs['Surface'],nodes['Diffuse BSDF'].outputs['BSDF'])
    return mat

# uv -> x -> colorramp -> diffuse shader
# not useful for saving individual segment color. limit of colors is 32.
def createColorRampEdgeMaterial(colors, name = 'splineColorMat', interpolation = 'LINEAR'):
    nrColor = colors.shape[0]
    mat=createMaterial(name)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.new(type='ShaderNodeBsdfDiffuse')
    coord = nodes.new(type='ShaderNodeTexCoord')
    vec2xyz = nodes.new(type='ShaderNodeSeparateXYZ')
    links.new(coord.outputs['UV'], vec2xyz.inputs[0])
    cRamp = nodes.new(type='ShaderNodeValToRGB')
    cr = cRamp.color_ramp
    cr.interpolation = interpolation
    for i in range(1, nrColor-1):
        cr.elements.new(i/(nrColor-1))
        cr.elements[i].color = colors[i]
    cr.elements[0].color = colors[0]
    cr.elements[-1].color = colors[-1]
    links.new(vec2xyz.outputs['X'], cRamp.inputs['Fac'])
    links.new(nodes['Diffuse BSDF'].inputs['Color'],cRamp.outputs['Color'])
    links.new(nodes['Material Output'].inputs['Surface'],nodes['Diffuse BSDF'].outputs['BSDF'])
    return mat

#####################
# material: world
#####################
def createWorldTexture(world, imgfile):
    #world.cycles_visibility.diffuse = False
    world.use_nodes = True
    img = bpy.data.images.load(imgfile)
    nodes = world.node_tree.nodes
    links = world.node_tree.links
    envtex = nodes.new('ShaderNodeTexEnvironment')
    envtex.image = img
    links.new(envtex.outputs['Color'], nodes['Background'].inputs['Color'])
    return
#####################
# Image and UV
#####################
    
## generate image from rgba matrix
#   name: image file name
#   rgba: width x height x 4 (RGBA)
#   return image: width (x) x height (y)
def createImage(name, rgba):
    #if name in bpy.data.images.keys():
    #    bpy.data.images.remove(bpy.data.images[name])        
    img = bpy.data.images.new(name, width=rgba.shape[0], height=rgba.shape[1], 
                            alpha=True, float_buffer=False)
    #img = bpy.data.images[name]         
    img.pixels = np.transpose(rgba, [1, 0, 2]).flatten('C')
    return img

## generate image and save to file to keep record
# after saving, load again to image datablock and return.
def createExternalImage(name, path, rgba):
    filename = path+name+'.png'
    rgb = rgba[:,:,:3]*255
    pixels = np.transpose(rgb, [1, 0, 2])
    image = Image.fromarray(pixels.astype('uint8'), 'RGB')
    image.save(filename)        
    img = bpy.data.images.load(filename)
    return img

# display uv layer information.
# example of how to iterate through UV layer.    
def showUVInfo(obj):
    me = obj.data
    uv_layer = me.uv_layers.active.data
    for poly in me.polygons:
        print("Polygon index: %d, length: %d" % (poly.index, poly.loop_total))
        for loop_index in range(poly.loop_start, poly.loop_start + poly.loop_total):
            print("    Vertex: %d" % me.loops[loop_index].vertex_index)
            print("    UV: %r" % uv_layer[loop_index].uv)
            

# set all UV points to (0, 0)
# This is used for mapping uv for UV_WARP modifier.
# see function visual_graph_skin for usage.
def zeroUV(obj):
    me = obj.data
    uv_layer = me.uv_layers[0].data
    for uv_point in uv_layer:
        uv_point.uv = [0,0]                      

            
## Swap out image files from material created using
#    createPlaneTextureFromImage(name, imgTexture)
# mat: material datablock
# imgTexture: image datablock
def updateMaterialImage(mat, imgTexture):
    imageShader = mat.node_tree.nodes.get('Image Texture')
    if imageShader:
        imageShader.image = imgTexture
        return True
    else:
        return False

#####################
# Color mapping
#####################

# create vector color layer and apply color to polygons
def create_vertex_color(name, ob, colors):
    # create vertex color layer
    vert_color = ob.data.vertex_colors.get(name)
    if not vert_color:
        vert_color = ob.data.vertex_colors.new(name=name)
    for pol in ob.data.polygons:
        for loop_id in pol.loop_indices:
            vi = ob.data.loops[loop_id].vertex_index
            vert_color.data[loop_id].color = colors[vi]
    return vert_color
    
# X: n-d array to map to [0,1]
# period: whether to rotate if Xij is beyond xmin/xmax
def mapToFactor(X, xmin=None, xmax=None, log=False, period = None):
    if xmin is None:
        xmin = np.min(X)
    if xmax is None:
        xmax = np.max(X)
    if xmin >= xmax:
        raise ValueError('min >= max')
    if log and (xmin < 0):
        raise ValueError('min < 0, cannot use np.log()')   
    if log:
        X[X<=xmin] = xmin 
        X = np.log(X)
        xmin = np.log(xmin)
        xmax = np.log(xmax)
    # interpolate color map break points to [xmin, xmax]
    fac = np.interp(X, [xmin, xmax], [0,1], period = period)
    return fac

# X: n-d array to map to color space
# color map: m (min, mid1, ... , midm-2, max) by 4 (RGBA, or HSVA) matrix
def mapToColor(X, colormap, xmin=None, xmax=None, log=False, maptype='RGBA'):
    X_fac = mapToFactor(X, xmin, xmax, log, None)
    nrSeg = colormap.shape[0]-1
    xp_fraction = np.arange(0, 1+1/nrSeg, 1/nrSeg)
    if maptype == 'RGBA':
        rgba = mapFromRGBA(X_fac, xp_fraction, colormap)
    elif maptype == 'HSVA':
        raise ValueError('maptype HSVA turned off in this version' )
    return rgba

# mapping (xp, colormap), evaluated at X
# X: m by n, 
# xp: k by 1, locations of colormap elements [xp_0 = xmin, ..., xp_k-1 = xmax]
# colormap: k by 4
# return
def mapFromRGBA(X, xp, colormap):
    R = np.interp(X, xp, colormap[:,0])
    G = np.interp(X, xp, colormap[:,1])
    B = np.interp(X, xp, colormap[:,2])
    A = np.interp(X, xp, colormap[:,3])
    return np.stack((R,G,B,A), -1)


#def mapFromHSVA(X, xp, colormap):
#    H = np.interp(X, xp, colormap[:,0])
#    S = np.interp(X, xp, colormap[:,1])
#    V = np.interp(X, xp, colormap[:,2])
#    A = np.interp(X, xp, colormap[:,3])
#    hsv = np.stack((H,S,V), -1)
#    rgb = mplc.hsv_to_rgb(hsv)
#    a = np.expand_dims(A, axis=-1)
#    return np.concatenate((rgb,a), axis=-1)

##########################
## purge orphan data
##########################
# unused data, when enough accumulated, slow down 
# computation significantly. 
# Orphan data can be cleared from outliner, or one 
# of the functions below.
# Not a concern when using headless mode.

# very slow.
def removeOrphanData():
    
    for block in bpy.data.worlds:
        if block.users == 0:
            bpy.data.worlds.remove(block)
            
    for block in bpy.data.meshes:
        if block.users == 0:
            bpy.data.meshes.remove(block)
            
    for block in bpy.data.curves:
        if block.users == 0:
            bpy.data.curves.remove(block)
    
    for block in bpy.data.materials:
        if block.users == 0:
            bpy.data.materials.remove(block)
    
    for block in bpy.data.textures:
        if block.users == 0:
            bpy.data.textures.remove(block)
    
    for block in bpy.data.images:
        if block.users == 0:
            bpy.data.images.remove(block)

# may mess up context. not really useful.
def removeOrphanData_purge(filepath=bpy.data.filepath):
    for i in range(4):
        bpy.ops.wm.save_as_mainfile(filepath=filepath)
        bpy.ops.wm.open_mainfile(filepath=filepath)

    
##########################
## Rotation (XYZ Euler)
##########################

# assuming camera top/bottom is perpendicular
# to Z axis, and we want object (such as text) y axis to # be parallel to camera left/right edge while z axis
# point into camera.
def find_camera(ob, cam):
    abs_loc_ob = np.asarray(ob.matrix_world)[:3,3]
    abs_loc_cam = np.asarray(cam.matrix_world)[:3,3]
    newz = abs_loc_cam-abs_loc_ob
    rot = vecPairToXYZEulerRot(newz, [0,0,1], d='y')
    ob.rotation_euler = rot


# get xyz euler rotation to new vz
# for cylindrical symmetry items where the other two axes don't matter
def zVecToXYZEulerRot(vz):
    z0 = np.asarray([0,0,1])
    rot = np.asarray([0,0,0])
    if np.dot(z0, vz) == 0:
        return rot
    else:
        # get the second axis. can be anything not parallel to vz,
        # but we use z0 here because it's now known it qualifies
        v1 = z0 - np.dot(vz, z0)/np.dot(vz, vz)*vz
        rot = vecPairToXYZEulerRot(vz, v1)
        return rot
    
# calculate euler rotation into new system.
# input: 
# vz: new z axis
# v1: orthogonal vectors to vz
# d: default : 'x': v1 is new vx 
#   if d == 'y': v1 is new vy 
def vecPairToXYZEulerRot(vz, v1, d='x'):
    
    assert (d in ['x', 'y']), "d must be 'x' or 'y' "
    vx = v1
    vy = np.cross(vz, v1)
    
    if d == 'y':
        vx, vy = - vy, vx
        
    R0 = np.column_stack((vx,vy,vz))
    R = R0 / np.linalg.norm(R0, axis = 0)
    rot = rotationMatrixToXYZEulerAngles(R)      
    return rot

# calculate eular angles from rotation matrix
def rotationMatrixToXYZEulerAngles(R):
    sy = math.sqrt(R[0,0] * R[0,0] +  R[1,0] * R[1,0])
    singular = sy < 1e-6
    if  not singular :
        x = math.atan2(R[2,1] , R[2,2])
        y = math.atan2(-R[2,0], sy)
        z = math.atan2(R[1,0], R[0,0])
    else :
        x = math.atan2(-R[1,2], R[1,1])
        y = math.atan2(-R[2,0], sy)
        z = 0
    return np.array([x, y, z])

################################
# Open Shading Language related
################################

def create_OSL_script(name, values, digits = None):
    nrValue = values.shape[0]
    f_format = ''
    if digits is None:
        f_format = '{}'
    else:
        f_format = '{{0:.{}f}}'.format(digits)
    t = bpy.data.texts.new(name)
    t.write('shader value( int id = 0, output float Val = 1){\n')
    t.write('float vec[{}]={{\n'.format(nrValue))
    for v in values[:-1]:
        t.write(f_format.format(v)+',\n')
    t.write(f_format.format(values[-1])+'};\n')
    t.write('Val = vec[id];}')
    return t
