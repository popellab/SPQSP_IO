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
cellFileName = working_dir + '/data/sampleCell.csv'
with open(cellFileName, 'r') as f:
    reader = csv.reader(f)
    # skip header
    next(reader)
    cellfiledata = np.asarray(list(reader)).astype(float)        
        
# vessels
graphOutfile = working_dir + '/data/sampleNet.xml'
tree = ET.ElementTree()
tree.parse(graphOutfile)
graphData = tree.getroot()

el = graphData.find('vertex').text
v = np.asarray([s.split(',') for s in el.strip('\n').split('\n')])[:,0:3].astype(float)

el = graphData.find('edge').text
e = np.asarray([s.split(',') for s in el.strip('\n').split('\n')])[:,0:2].astype(int)


##########################
## WORLD
##########################
#render
bu.render_config(x=320, y=320, sample=256, use_gpu = True)

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
objectSize = np.asarray([4, 4, 4])
fieldCenter = objectSize/2

studio.set_location(fieldCenter)
studio.set_subject_size(5)
studio.adjust_light('key', rot_x=np.pi/3)
studio.adjust_light('fill',rot_x=np.pi/3)
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
objDir = str(path_repo) +'/resource/objects/'

obj_file = objDir + 'DC.obj'
if not bpy.data.objects.get('DCPrototype'):
    proto_DC = bu.importObj(obj_file, 'DCPrototype')
    proto_DC.scale *= .2
    proto_DC.location = np.asarray([-3, 0, 0])
    sce.collection.objects.unlink(proto_DC)
    sce_sp.collection.objects.link(proto_DC)

obj_file = objDir + 'fb_0.obj'
if not bpy.data.objects.get('FBPrototype'):
    proto_fb = bu.importObj(obj_file, 'FBPrototype')
    proto_fb.scale *= .2
    proto_fb.location = np.asarray([-4, 0, 0])
    sce.collection.objects.unlink(proto_fb)
    sce_sp.collection.objects.link(proto_fb)

obj_file = objDir + 'mac_0.obj'
if not bpy.data.objects.get('Mac0Prototype'):
    proto_m0 = bu.importObj(obj_file, 'Mac0Prototype')
    proto_m0.scale *= .5
    proto_m0.location = np.asarray([-5, 0, 0])
    sce.collection.objects.unlink(proto_m0)
    sce_sp.collection.objects.link(proto_m0)

obj_file = objDir + 'mac_1.obj'
if not bpy.data.objects.get('Mac1Prototype'):
    proto_m1 = bu.importObj(obj_file, 'Mac1Prototype')
    proto_m1.scale *= .2
    proto_m1.location = np.asarray([-6, 0, 0])
    sce.collection.objects.unlink(proto_m1)
    sce_sp.collection.objects.link(proto_m1)


loc_ffa = np.asarray([-2,0,0])
r = 0.02
if not bpy.data.objects.get('cellPrototype'):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=r,location=loc_ffa)
    proto_c = bpy.context.object
    proto_c.dimensions=(.5,.4,.3)
    proto_c.name = 'cellPrototype'
    sce.collection.objects.unlink(proto_c)
    sce_sp.collection.objects.link(proto_c)
     
################################
# Visualized data
################################

# add box
box = be.visual_box('box', fieldCenter, objectSize, 0.01, (0,1,0,1))
   
   
# network
r = .05
vas = bc.network_curve_mono('vas', sce, v, e, r, (1,.2,.2,1))


# cells
crd = cellfiledata[cellfiledata[:,3] == 1,:3]
cell_d = bc.cell_mono('cell_d', sce, proto_DC, crd, (.4,1,.4,1))

crd = cellfiledata[cellfiledata[:,3] == 2,:3]
cell_fb = bc.cell_mono('cell_f', sce, proto_fb, crd, (.1,.6,1,1))

crd = cellfiledata[cellfiledata[:,3] == 3,:3]
cell_m0 = bc.cell_mono('cell_m', sce, proto_m0, crd, (.5,.2,1,1))

crd = cellfiledata[cellfiledata[:,3] == 4,:3]
t_property = np.random.rand(sum(cellfiledata[:,3] == 4),)
colorMapRYW = np.asarray([[1,0,0,1],[1,1,0,1],[1,1,1,1]])    
colors = be.mapToColor(t_property, bu.colormapRYW, maptype='RGBA')

cell_t = bc.cell_color('cell_t', sce, proto_c, crd, colors)
#cell_t.update_cells(crd, colors)
    
#############################
# rendering
#############################

scene = bpy.context.scene
scene.frame_start = 0
scene.frame_end = 20
frames = list(range(0,scene.frame_end+1))

for frame_nr in frames:
    scene.frame_set(frame_nr)
    f = scene.frame_current
    f0 = scene.frame_start
    f1 = scene.frame_end
    studio.set_rotation(0, np.interp(f, [f0, f1], [-np.pi/12, np.pi*1/12]))
    scene.view_layers[0].update()# for children object to transform   
    arrow.reset_location()
    img_filename = '{}'.format(frame_nr)
    sce.render.filepath = working_dir+'/scratch/abm/'+img_filename
    bpy.ops.render.render(write_still=True)