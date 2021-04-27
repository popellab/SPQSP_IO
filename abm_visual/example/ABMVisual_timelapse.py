import sys
import importlib
import bpy
from pathlib import Path

# running as a script from terminal:
path_script = Path(__file__)

# running as a script from within blender:
#path_script = Path(bpy.context.space_data.text.filepath)

path_repo = path_script.parent.parent
sys.path.append(str(path_repo.joinpath('LIB')))

import numpy as np
import math
import csv
#import lxml.etree as ET
import xml.etree.ElementTree as ET 

from BlenderVisual import blender_element as be
from BlenderVisual import blender_composite as bc
from BlenderVisual import blender_utility as bu

importlib.reload(bc)
importlib.reload(be)
importlib.reload(bu)


#############################
### input
#############################

working_dir = str(path_repo) + '/example'

# cells
lastSlice = 80
slicePerSnapshot = 40
slicePerDay = 4
t_slice = np.arange(0,lastSlice+1,slicePerSnapshot)


def getCellData(slice):
    cellFileName = working_dir + '/data/abm/snapshots/cell_{}.csv'.format(slice)
    print(cellFileName)
    header, data = bu.get_csv_data(cellFileName, 1)
    crd = data[:,:5].astype(float)
    extra=np.array([x.replace('"','') for x in data[:,-1]])
    return crd, extra

celldata, cellextra = getCellData(t_slice[-1])

##########################
## WORLD
##########################
#render
bu.render_config(x=720, y=720, sample=128, use_gpu = True)

world = bu.reset_world()
#world.color = (0.05,0.05,0.05)
world.color = (1,1,1)
sce = bpy.data.scenes[0]
sce_sp = bpy.data.scenes[1]

background_file = str(path_repo) + '/resource/background/tissue_2.png'
be.createWorldTexture(world, background_file)

##########################
## Studio
##########################

studio = bc.photo_studio(sce)

# stage
objectSize = np.asarray([100, 100, 100])
fieldCenter = objectSize/2

studio.set_location(fieldCenter)
studio.set_rotation(0, np.pi/6*5)
studio.set_subject_size(100)
studio.adjust_light('key', strength = 8e4, rot_x=np.pi/3)
studio.adjust_light('fill', strength = 1e4, rot_x=np.pi/3)
studio.adjust_light('rim', strength = 5e4, rot_x=np.pi/3)

mat_black = be.createEmissionMaterial('k_emission', [0,0,0,1])

anchor_axis = studio.create_camera_anchor('axis_anchor', [9, -10, -60])
arrow = bc.axis_arrow(sce, 2) 
arrow.set_anchor(anchor_axis)
arrow.reset_location()
arrow.set_mat(mat_black)
    
wm_text = 'ABM visual example'
anchor_wm = studio.create_camera_anchor('wm_anchor', [-10, 10, -60])
wm = bc.watermark(sce, wm_text, mat_black)
wm.set_anchor(anchor_wm)
    


##########################
## SETUP
##########################

# import obj
def get_proto_ellipsoid(name, dim):
    loc_ffa = np.asarray([0,0,0])
    if not bpy.data.objects.get(name):
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, location=loc_ffa)
        proto = bpy.context.object
        proto.dimensions = dim
        proto.name = name
        sce.collection.objects.unlink(proto)
        sce_sp.collection.objects.link(proto)
        return proto
    else:
        return bpy.data.objects.get(name)

proto_cp = get_proto_ellipsoid('CancerPosProto', (1.8,1.6,1.3))
proto_cn = get_proto_ellipsoid('CancerNegProto', (1.3,1.2,1))
proto_t = get_proto_ellipsoid('TPrototype', (.5,.5,.5))
    
################################
# Visualized data
################################
# add box
box = be.visual_box('box', fieldCenter, objectSize, 0.01, (1,1,1,1))
   
#cells
crd_cp = celldata[np.logical_and(celldata[:,3] == 1, celldata[:,4] == 1),:3]
cp = bc.cell_mono('cancer_p', sce, proto_cp, crd_cp, (.9,.1, 0,1))
cp.set_render(preview = False, render=True)

    
crd_cn = celldata[np.logical_and(celldata[:,3] == 1, celldata[:,4] == 2),:3]
cn = bc.cell_mono('cancer_n', sce, proto_cn, crd_cn, (.4,.6,.95,1))
cn.set_render(preview = False, render=True)

crd_t = celldata[celldata[:,3] == 2,:3]
tc = bc.cell_mono('cell_t', sce, proto_t, crd_t, (0.3,.9,0.3,1))
tc.set_render(preview = False, render=True)


#############################
# rendering
#############################

def jiggleCell(c):
    r = np.random.rand(*c.shape)*1
    c = c+r
    return c

cp.update_cells(jiggleCell(crd_cp))
cn.update_cells(jiggleCell(crd_cn))
tc.update_cells(jiggleCell(crd_t))

# events
def visualSlice(scene):    
    f = scene.frame_current
    if f > len(t_slice):
        return
    t = t_slice[f]
    celldata, cellextra= getCellData(t)
    #cut
    celldata = celldata[celldata[:,1]<=celldata[:,0],:]
    crd_cp = celldata[np.logical_and(celldata[:,3] == 1, celldata[:,4] == 1),:3]
    crd_cn = celldata[np.logical_and(celldata[:,3] == 1, celldata[:,4] == 2),:3]
    crd_t = celldata[celldata[:,3] == 2,:3]
    cp.update_cells(jiggleCell(crd_cp))
    cn.update_cells(jiggleCell(crd_cn))
    tc.update_cells(jiggleCell(crd_t))
    wm.set_text('T = {} days'.format(t/slicePerDay))
    # put here if want to do when changing frame
    #sce.update()
    arrow.reset_location()
    return


sce.frame_start = 0
sce.frame_end = len(t_slice)
#sce.frame_end = 180
sce.frame_current = 2

# movie
bpy.app.handlers.frame_change_pre.clear()
bpy.app.handlers.frame_change_pre.append(visualSlice)

sce.render.image_settings.file_format = 'JPEG'

frames = list(range(len(t_slice)))
for frame_nr in frames:
    sce.frame_set(frame_nr)
    sce.view_layers[0].update()
    arrow.reset_location()
    sce.render.filepath = working_dir+'/scratch/abm_time/'+ str(frame_nr)
    bpy.ops.render.render(write_still=True) # render still


