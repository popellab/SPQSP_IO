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


def jiggleCell(c):
    r = np.random.rand(*c.shape)*1
    c = c+r
    return c


celldata, cellextra = getCellData(t_slice[2])
celldata[:,:3] = jiggleCell(celldata[:,:3])

##########################
## WORLD
##########################
#render
bu.render_config(x=720, y=720, sample=128, use_gpu = False)

world = bu.reset_world()
world.color = (0.05,0.05,0.05)
#world.horizon_color = (1,1,1)
sce = bpy.data.scenes[0]
sce_sp = bpy.data.scenes[1]
##########################
## Studio
##########################
studio = bc.photo_studio(sce)

# stage
objectSize = np.asarray([100, 100, 100])
fieldCenter = objectSize/2

studio.set_location(fieldCenter)
studio.set_rotation(0, np.pi/4*3)
studio.set_subject_size(50)
studio.adjust_light('key', rot_x=np.pi/2, rot_z =np.pi/4*3)
studio.adjust_light('fill', rot_x=np.pi/3)
studio.adjust_light('rim', rot_x=np.pi/3)

mat_white = be.createEmissionMaterial('w_emission', [1,1,1,1])

anchor_axis = studio.create_camera_anchor('axis_anchor', [9, -10, -60])
arrow = bc.axis_arrow(sce, 2) 
arrow.set_anchor(anchor_axis)
arrow.reset_location()
#arrow.set_mat(mat_white)
    
wm_text = 'ABM visual example'
anchor_wm = studio.create_camera_anchor('wm_anchor', [-10, 10, -60])
wm = bc.watermark(sce, wm_text, mat_white)
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
proto_cn = get_proto_ellipsoid('CancerNegProto', (1.3,1.2,1.1))
proto_t = get_proto_ellipsoid('TPrototype', (.5,.5,.5))
    
################################
# Visualized data
################################


# add box
#box = be.visual_box('box', fieldCenter, objectSize, 0.01, (0,1,0,1))
   
   
#cells

crd_cp = celldata[np.logical_and(celldata[:,3] == 1, celldata[:,4] == 1),:3]
cp = bc.cell_mono('cancer_p', sce, proto_cp, crd_cp, (.9,.1, 0,1))
cp.set_render(preview = False, render=True)

    
crd_cn = celldata[np.logical_and(celldata[:,3] == 1, celldata[:,4] == 2),:3]
cn = bc.cell_mono('cancer_n', sce, proto_cn, crd_cn, (.2,.5,.9,1))
cn.set_render(preview = False, render=True)

crd_cut = celldata[np.logical_and(celldata[:,3] == 1, celldata[:,4] == 2),:3]
ccut = bc.cell_mono('cancer_cut', sce, proto_cn, crd_cn, (.2,.5,.9,1))
ccut.set_render(preview = True, render=True)


crd_t = celldata[celldata[:,3] == 2,:3]
tc = bc.cell_mono('cell_t', sce, proto_t, crd_t, (0.9,.9,0.5,1))
tc.set_render(preview = False, render=True)


#############################
# cutting
#############################

cut_loc = 20
bpy.ops.mesh.primitive_cube_add()
cube = bpy.context.object
cube.scale = (52,2,52)
cube.location = (50,cut_loc+2,50)
cube.hide_render = True
cube.hide_viewport = True

crd_cut = celldata[np.logical_and(celldata[:,1]>cut_loc-1, 
                    celldata[:,1]<cut_loc+1),:3]
celldata = celldata[celldata[:,1]<=cut_loc-1,:]

crd_cp = celldata[np.logical_and(celldata[:,3] == 1, celldata[:,4] == 1),:3]
crd_cn = celldata[np.logical_and(celldata[:,3] == 1, celldata[:,4] == 2),:3]
crd_t = celldata[celldata[:,3] == 2,:3]
cp.update_cells(crd_cp)
cn.update_cells(crd_cn)
tc.update_cells(crd_t)
ccut.update_cells(crd_cut)
    
for o in bpy.context.selected_objects:
    o.select_set(False)

scene = bpy.data.scenes[0]

dg = bpy.context.evaluated_depsgraph_get()
obj = bpy.data.objects['cancer_cut']
parts = obj.evaluated_get(dg).particle_systems[0].particles

print(len(parts))
cut_cube = bpy.data.objects['Cube']
cut_collection = bpy.data.collections.new("cut_coll")
scene.collection.children.link(cut_collection)
proto = proto_cn
for i, p in enumerate(parts):
    # duplicate object
    dupli = bpy.data.objects.new(name="particle_cut",object_data=proto.data)
    cut_collection.objects.link(dupli)
    # replicate particle properties
    dupli.rotation_quaternion = p.rotation
    dupli.location = p.location
    dupli.scale = proto.scale
    # cutting
    bool_one = dupli.modifiers.new(type="BOOLEAN", name="cut")
    bool_one.object = cube
    bool_one.operation = 'DIFFERENCE'

bpy.data.objects.remove(obj, do_unlink=True)

proto_cn.active_material = be.createGlossyMaterial('gloss', (.2,.5,.9,1))
#############################
# rendering
#############################


sce.render.image_settings.file_format = 'JPEG'

sce.render.filepath = working_dir+'/scratch/flat/'+ 'cut'
bpy.ops.render.render(write_still=True) # render still


