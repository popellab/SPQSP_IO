"""
BlenderVisual.blender_utility

Tools and shorthands, Code fragments, IOs, etc.

Created on Sat Sep 21 08:51:40 2019

@author: Chang Gong
"""
import numpy as np
import csv
import bpy

#%% data handling

def importObj(obj_file, name):
    bpy.ops.import_scene.obj(filepath=obj_file)
    obj = bpy.context.selected_objects[0]
    obj.name = name
    return obj


# read csv file data, return header and content
def get_csv_data(filename, header_line = 1):
    with open(filename) as csvfile:
        reader = csv.reader(csvfile, delimiter=',', quotechar='|')
        # header
        header = ''
        for i in range(header_line):
            header = next(reader)
        # data
        data = np.asarray(list(reader))
        return header, data
    
## interpolate from 3D, along slice_axis
def get_slice_interpolation(fn, slice_axis, slice_location):
    axis = [0,1,2]
    axis.pop(slice_axis)
    dim = [fn.values.shape[i] for i in axis]
    slice_plane = np.zeros([*dim, 3])
    for i in range(dim[0]):
        for j in range(dim[1]):
            slice_plane[i, j, axis[0]] = fn.grid[axis[0]][i]
            slice_plane[i, j, axis[1]] = fn.grid[axis[1]][j]
            slice_plane[i, j, slice_axis] = slice_location
    slice_plane_flat = slice_plane.reshape([-1,3])
    v = fn(slice_plane_flat).reshape(dim)
    return v

# fn is scipy.interpolate.RegularGridInterpolator
# cmin, cmax, nr_c: [x, y, z], min, max, total
def get_interpolation_from_3D(fn, axis, cmin, cmax, nrc):
    X,Y,Z = np.mgrid[cmin[0]:cmax[0]:1j*nrc[0],
                     cmin[1]:cmax[1]:1j*nrc[1],
                     cmin[2]:cmax[2]:1j*nrc[2]]
    xyz = np.stack((X,Y,Z), -1).reshape([-1,3])
    value = fn(xyz).reshape(X.shape)
    value_2d = np.squeeze(value, axis=axis)
    #print(value_2d.shape)
    return value_2d

# get interpolated location from slice id. 
# nr_slice: data dimensions
def get_interp_loc_from_slice(x, nr_slice, range_loc):
    step = (range_loc[1]-range_loc[0])/nr_slice
    loc = np.interp(x, [0, nr_slice-1], [range_loc[0]+step/2, range_loc[1]-step/2])
    return loc
    
# draw: ImageDraw.Draw(image)
# cell: list of cells to draw. [x, y, radius, r, g, b]
# lengths are in unit of pixels
def draw_IHC(draw, cells):
    # preprocessing
    bound = np.array([ [x-r, y-r, x+r, y+r]  for (x, y, r) in cells[:,:3]])
    color = (cells[:, 3:]*255).astype(int)
    # PIL image
    # draw cells on image
    for i, c in enumerate(color):
        draw.ellipse(tuple(bound[i]), fill=tuple(c))
    # convert image to rgba array for blender to use
    return
    
def clear_object_list(obs, remove_mesh = True):
    ob_name_list = [o.name for o in obs]# otherwise, obs might be changed during iteration 
    for n in ob_name_list:
        ob = bpy.data.objects.get(n)
        if ob and ob.rna_type.identifier == 'Object':
            _data = ob.data
            bpy.data.objects.remove(ob)#, do_unlink = True
            if remove_mesh and _data:# in case of empty object
                if _data.rna_type.identifier == 'Mesh':
                    bpy.data.meshes.remove(_data)
    return

def reset_world():
    # remove existing objects
    obs = bpy.data.objects
    clear_object_list(obs)
    
    nrScene = len(bpy.data.scenes)
    
    if nrScene < 2:
        for i in range(2-nrScene):
            bpy.data.scenes.new('scene')
    else:
        for s in bpy.data.scenes[2:]:
            bpy.data.scenes.remove(s, do_unlink=True)
        
    scene = bpy.data.scenes[0]
    scene.name = 'main'
    sce_sp = bpy.data.scenes[1]
    sce_sp.name = 'supportScene'
    #bpy.data.screens['Visual'].scene = sce
      
    if not scene.world:
        newWorld = bpy.data.worlds.new('world')
        scene.world = newWorld
    scene.world.name = 'world'
    return scene.world


def render_config(x, y, sample, use_gpu = False):
    for scene in bpy.data.scenes:
        scene.render.engine = 'CYCLES'
    scene = bpy.data.scenes[0]
    scene.render.resolution_x = x
    scene.render.resolution_y = y
    scene.render.tile_x = 128
    scene.render.tile_y = 128
    scene.render.resolution_percentage = 100
    scene.cycles.caustics_reflective = False
    scene.cycles.caustics_refractive = False
    scene.cycles.blur_glossy = 1
    scene.cycles.max_bounces = 4
    scene.cycles.min_bounces = 0
    scene.cycles.samples = sample
    if use_gpu:
        scene.cycles.device = 'GPU'
    else:
        scene.cycles.device = 'CPU'
    return
    
#%% color
colormapRYW = np.asarray([[1,1,1,1],[1,1,0,1],[1,0,0,1]])
colormapBGR = np.asarray([[0,0,1,1],[0,1,1,1],[0,1,0,1],[1,1,0,1],[1,0,0,1]])
colormapBWR = np.asarray([[0,0,1,1],[.5,.5,1,1],[1,1,1,1],[1,.5,.5,1],[1,0,0,1]])
colormapRainbow = np.asarray([[1,0,0,1],# red
                              [1,.33,0,1],
                              [1,.5,0,1],# orange
                              [1,.67,0,1],
                              [1,1,0,1],# yellow
                              [.67,1,0,1],
                              [0,1,0,1],# green
                              [0,1,.67,1],
                              [0,1,1,1],# cyan
                              [0,.67,1,1],
                              [0,0,1,1],# blue
                              [.5,0,1,1],
                              [1,0,1,1]])

cv = '''<color>#440154FF </color>
<color>#481567FF </color>
<color>#482677FF </color>
<color>#453781FF </color>
<color>#404788FF </color>
<color>#39568CFF </color>
<color>#33638DFF </color>
<color>#2D708EFF </color>
<color>#287D8EFF </color>
<color>#238A8DFF </color>
<color>#1F968BFF </color>
<color>#20A387FF </color>
<color>#29AF7FFF </color>
<color>#3CBB75FF </color>
<color>#55C667FF </color>
<color>#73D055FF </color>
<color>#95D840FF </color>
<color>#B8DE29FF </color>
<color>#DCE319FF </color>
<color>#FDE725FF </color>'''
cv_list = cv.split('\n')
viridis = []
for c in cv_list:
    color = []
    for i in [0,2,4,6]:
        h = c.split('#')[1].split(' ')[0][i:i+2]
        color.append(int(h, 16)/255)
    viridis.append(color)

colormapVIRIDIS = np.asarray(viridis)

colormapVIRIDIS_transparent = colormapVIRIDIS
colormapVIRIDIS_transparent[:,3]=np.arange(1,21)*.02+0.6

agent_color = {'red': (1, .05, .05,1),
               'green':(0.05,.9,0.1,1),
               'blue': (.05, .2, 1, 1),
               'pink': (.95,.5,.5,1),
               'light_green': (0.4,.9,0.4,1),
               'light_blue':(.3, .55, .95, 1),
               'purple': (.5,.35,.95, 1),
               'orange': (.95,.25,.05, 1),
               'yellow':(.95, .85, 0.05, 1),
               'cyan':(0.05,.8,0.9,1),
               'brown': (.075, .05, .025, 1),
               'light_brown': (.54, .3, .18, 1)}
               