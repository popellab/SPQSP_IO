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

from skimage import measure

from BlenderVisual import blender_element as be
from BlenderVisual import blender_composite as bc
from BlenderVisual import blender_utility as bu

importlib.reload(bc)
importlib.reload(be)
importlib.reload(bu)

##########################
## input
##########################

working_dir = str(path_repo) + '/example'

#findsurf = measure.marching_cubes #older skimage
findsurf = measure.marching_cubes_classic

# generate surface
X,Y,Z = np.mgrid[-2:2:40j, -2:2:40j, -2:2:40j]
surf_eq = X**4 + Y**4 + Z**4 - (X**2+Y**2+Z**2)**2 + 3*(X**2+Y**2+Z**2) - 3

##########################
## WORLD
##########################
#render
bu.render_config(x=720, y=720, sample=32, use_gpu = False)

world = bu.reset_world()
world.color = (0,0,0)
#world.horizon_color = (1,1,1)
sce = bpy.data.scenes[0]
sce_sp = bpy.data.scenes[1]


##########################
## Studio
##########################
studio = bc.photo_studio(sce)

# stage
# stage
objectSize = np.asarray([4, 4, 4])
fieldCenter = objectSize/2

studio.set_location(fieldCenter)
studio.set_subject_size(5)
studio.adjust_light('key', strength = 3e3, rot_x=np.pi/3)
studio.adjust_light('fill', strength = 1e3, rot_x=np.pi/3)
studio.adjust_light('rim', strength = 2e3, rot_x=np.pi/3)

mat_black = be.createEmissionMaterial('k_emission', [0,0,0,1])

anchor_axis = studio.create_camera_anchor('axis_anchor', [9, -10, -60])
arrow = bc.axis_arrow(sce, 2) 
arrow.set_anchor(anchor_axis)
arrow.reset_location()
arrow.set_mat(mat_black)
    
wm_text = 'Isosurface'
anchor_wm = studio.create_camera_anchor('wm_anchor', [-10, 10, -60])
wm = bc.watermark(sce, wm_text, mat_black)
wm.set_anchor(anchor_wm)
    


                    
                    
##########################
# Iso surface 
##########################

# outermost
vertices, simplices = findsurf(surf_eq, -1)
verts = vertices * 4 / (40-1)
name = 'face0'
face = be.create_Faces (name, sce, verts, simplices)
be.smooth_shading(face)

# innermost
vertices, simplices = findsurf(surf_eq, 5)
verts = vertices * 4 / (40-1)
name = 'face1'
face = be.create_Faces (name, sce, verts, simplices)
be.smooth_shading(face)
    
# use color to control transparency
colorB = (.1, .5, 1,1)
colorR = (1, .05, .05 ,1)

matB = be.createXRayMaterial('matB', color = colorB, strength = .5, depth=.1, weight=0.025)
bpy.data.objects['face0'].active_material = matB

matR = be.createXRayMaterial('matR', color = colorR)
bpy.data.objects['face1'].active_material = matR

# add box
box = be.visual_box('box', fieldCenter, objectSize, 0.01, (1,.3,0,1))


#############################
# rendering
#############################
sce.render.image_settings.file_format = 'JPEG'

sce.frame_start = 0
sce.frame_end = 20
sce.frame_current = sce.frame_start

cf = sce.frame_start
studio.set_rotation(0, 0)
studio._stage_center.keyframe_insert(data_path='rotation_euler', frame=(cf))

cf = sce.frame_end
studio.set_rotation(0, np.pi/2)
studio._stage_center.keyframe_insert(data_path='rotation_euler', frame=(cf))

bpy.app.handlers.frame_change_pre.clear()
bpy.app.handlers.frame_change_pre.append(lambda _:arrow.reset_location())

frames = list(range(sce.frame_end+1))
for frame_nr in frames:
    sce.frame_set(frame_nr)
    arrow.reset_location()
    sce.render.filepath =  working_dir +'/scratch/surf2/' + str(frame_nr)
    bpy.ops.render.render(write_still=True)





