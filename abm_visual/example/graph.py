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

import time
start = time.time()

##########################
## WORLD
##########################
#render
bu.render_config(x=720, y=720, sample=32, use_gpu = False)

world = bu.reset_world()
world.color = (0,0,0)
#world.color = (1,1,1)
sce = bpy.data.scenes[0]
sce_sp = bpy.data.scenes[1]


##########################
## Studio
##########################
studio = bc.photo_studio(sce)

# stage
objectSize = np.asarray([10, 10, 10])
fieldCenter = objectSize/2
studio.set_location(fieldCenter)
studio.set_subject_size(10)
studio.adjust_light('key', strength = 5e2, rot_x=np.pi/3)
studio.adjust_light('fill', rot_x=np.pi/3)
studio.adjust_light('rim', rot_x=np.pi/3)

mat_white = be.createEmissionMaterial('w_emission', [1,1,1,1])

anchor_axis = studio.create_camera_anchor('axis_anchor', [9, -10, -60])
arrow = bc.axis_arrow(sce, 2) 
arrow.set_anchor(anchor_axis)
arrow.reset_location()
arrow.set_mat(mat_white)
    
wm_text = 'Tumor vasculature'
anchor_wm = studio.create_camera_anchor('wm_anchor', [-10, 10, -60])
wm = bc.watermark(sce, wm_text, mat_white)
wm.set_anchor(anchor_wm)
    

##########################
## Graph 
##########################

scene = bpy.context.scene
    
# vessels
working_dir = str(path_repo) + '/example'

USE_EXTERNAL_GRAPH = True
if USE_EXTERNAL_GRAPH:
    graphOutfile = working_dir + '/data/TumorVas.xml'

    tree = ET.ElementTree()
    tree.parse(graphOutfile)
    graphData = tree.getroot()

    el = graphData.find('vertex').text
    v_data = np.asarray([s.split(',') for s in el.strip('\n').split('\n')]).astype(float)
    v = (v_data[:,0:3]-np.array([2500,1500,1000]))/400
    col_vertex = graphData.find('col_vertex').text.split(',')

    el = graphData.find('edge').text
    e_data = np.asarray([s.split(',') for s in el.strip('\n').split('\n')]).astype(float)
    e = e_data[:,0:2].astype(int)
    col_edge = graphData.find('col_edge').text.split(',')

    fMin = min(v_data[:,4])
    fMax = np.percentile(v_data[:,4], 99)
    colors_val = (v_data[:,4]-fMin)/(fMax-fMin)

    thickness = v_data[:,3]/200
else:
    # test curve    
    v = np.asarray([[0,0,0], [0,3,0],[1,1,0],[1,0,1], [1,-1,0],[2,0,0],[3,-1,0]])
    e = np.asarray([[0,1],[1,2],[2,3],[0,3],[3,5],[4,6]])

    # v property
    vProp = np.zeros((v.shape[0], 2))
    vProp[:,0] = np.random.rand(v.shape[0],)**2
    vProp[:,1] = np.arange(1,8)/10
    
    fMin = min(vProp[:,0])
    fMax = max(vProp[:,0])
    colors_val = (vProp[:,0]-fMin)/(fMax-fMin)   
    thickness = vProp[:,1]

box = be.visual_box('box', fieldCenter, objectSize, 0.01, (0,1,0,1))

colors_fac= be.mapToFactor(colors_val, xmin = 0.0001, xmax = 1, log = True)

print('before visual graph: ' ,time.time() - start)  



#network = bc.network_curve_mono('vas', sce, v, e, thickness, [0,.5,1,1])
#network = bc.network_curve_color('vas', sce, v, e, thickness, bu.colormapRYW, colors_fac)
#network = bc.network_skin_mono('vas', sce, v, e, thickness, [0,.5,1,1])
network = bc.network_skin_color('vas', sce, v, e, thickness, bu.colormapRYW, colors_fac)


scene_ready = time.time()

scene.frame_start = 0
scene.frame_end = 20
frames = list(range(scene.frame_end+1))

for frame_nr in frames:
    scene.frame_set(frame_nr)
    f = scene.frame_current
    f0 = scene.frame_start
    f1 = scene.frame_end + 1
    studio.set_rotation(0, np.interp(f, [f0, f1], [-np.pi/6, np.pi*1/6]))
    scene.view_layers[0].update()# for children object to transform   
    arrow.reset_location()
    img_filename = '{}'.format(frame_nr)
    sce.render.filepath = working_dir+'/scratch/vas/'+img_filename
    bpy.ops.render.render(write_still=True)
    