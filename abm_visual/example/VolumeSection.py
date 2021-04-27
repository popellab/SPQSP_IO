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

from BlenderVisual import blender_element as be
from BlenderVisual import blender_composite as bc
from BlenderVisual import blender_utility as bu

importlib.reload(bc)
importlib.reload(be)
importlib.reload(bu)

##########################
## WORLD
##########################
#render
bu.render_config(x=1080, y=720, sample=32, use_gpu = False)

world = bu.reset_world()
world.color = (1,1,1)
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
studio.set_subject_size(6)
studio.adjust_light('key', rot_x=np.pi/3)
studio.adjust_light('fill', rot_x=np.pi/3)
studio.adjust_light('rim', rot_x=np.pi/3)
studio.set_rotation(0, np.pi/8*7)

mat_black = be.createEmissionMaterial('k_emission', [0,0,0,1])
anchor_axis = studio.create_camera_anchor('axis_anchor', [-.9, -.9, -8])
arrow = bc.axis_arrow(sce, .15) 
arrow.set_anchor(anchor_axis)
arrow.reset_location()
arrow.set_mat(mat_black)
    
wm_text = 'Cross section'
anchor_wm = studio.create_camera_anchor('wm_anchor', [-1.4, .9, -8])
wm = bc.watermark(sce, wm_text, mat_black)
wm.set_anchor(anchor_wm)
wm.set_scale(.1)
    

           
#####################
# data
#####################
working_dir = str(path_repo) + '/example'

from scipy.interpolate import RegularGridInterpolator

X,Y,Z = np.mgrid[-2:2:40j, -2:2:40j, -2:2:40j]
#surf_eq = X**3 - Y + Z**2 - (X**2+Y**2+Z**2)**2 + 3*(X**2+Y**2+Z**2) - 3
surf_eq = X**4 + Y**4 + Z**4 - ((X**2+Y+Z**3)**2)**1.5 + (X+Y+Z) - 3


x = np.arange(surf_eq.shape[0])
y = np.arange(surf_eq.shape[1])
z = np.arange(surf_eq.shape[2])
        
fn = RegularGridInterpolator((x,y,z), surf_eq)

#####################
# objects
#####################


colormap = bu.colormapVIRIDIS
#smin = np.percentile(surf_eq.flatten(), 20)
#smax = np.percentile(surf_eq.flatten(), 95)

smin = -8
smax = 8

#be.create_color_bar(colormap, smin, smax, 5, 'g/L')
cbar = bc.colorbar(sce, colormap, smin, smax)
cbar.annotate(5, 'g/L', '')
cbar.resize_text(1)
anchor_cbar = studio.create_camera_anchor('cbar_anchor', [-1.3, -.2, -8])
cbar.set_anchor(anchor_cbar)
cbar.set_scale(.15)
cbar.set_brightness(4)


init_bin = [.2, 0, .2]
init_cross = [bu.get_interp_loc_from_slice(b, 40, [0,4]) for b in init_bin]

vol = bc.box_slice('vol', sce, objectSize, (1,.3,0,1))
s0 = vol.add_slice('x', 0, init_cross[0])
s1 = vol.add_slice('y', 1, init_cross[1])
s2 = vol.add_slice('z', 2, init_cross[2])
vol.set_divider(init_cross, radius = .02, color = (1,1,1,1))

x_cut = np.interp(init_cross[0], [0,objectSize[0]] , [0, 39])
values_yz = bu.get_interpolation_from_3D(fn, 0, [x_cut,0,0], 
                                    [x_cut,39,39],[1,100,100])
rgba = be.mapToColor(values_yz, colormap, xmin=smin, xmax= smax,
            log=False, maptype='RGBA')
img = be.createImage('x_img', rgba)
vol.update_plane(s0, img, 0, init_cross[0])

z_cut = np.interp(init_cross[2], [0,objectSize[2]] , [0, 39])
values_xy = bu.get_interpolation_from_3D(fn, 2, [0,0,z_cut], 
                                    [39,39,z_cut],[100,100,1])
rgba = be.mapToColor(values_xy, colormap, xmin=smin, xmax= smax,
            log=False, maptype='RGBA')
img = be.createImage('z_img', rgba)
vol.update_plane(s2, img, 2, init_cross[2])
                                    
#####################
# slice
#####################
def visualSlice(scene):    
    f = scene.frame_current
    f0 = scene.frame_start
    f1 = scene.frame_end
    y_min = 0
    y_max = 39
    y_slice = np.interp(f, [f0, f1], [y_min, y_max])
    values_xz = bu.get_interpolation_from_3D(fn, 1, [0,y_slice,0], 
                                    [39,y_slice,39],[100,1,100])
    values_zx = np.transpose(values_xz)
    rgba = be.mapToColor(values_zx, colormap, xmin=smin, xmax= smax,log=False, maptype='RGBA')
    img = be.createImage('y{}'.format(f), rgba)
    loc = bu.get_interp_loc_from_slice(y_slice, 40, [0,4])
    vol.update_plane(s1, img, 1, loc)
    wm.set_text('y = {:.2f}'.format(y_slice))
    
## animation
bpy.app.handlers.frame_change_pre.clear()
bpy.app.handlers.frame_change_pre.append(visualSlice)


sce.frame_start = 1
sce.frame_end = 40

sce.frame_current = sce.frame_start
sce.render.image_settings.file_format = 'JPEG'

frames = list(range(1,sce.frame_end+1))
#frames = list(range(1, 2))
for frame_nr in frames:
    sce.frame_set(frame_nr)
    arrow.reset_location()
    sce.render.filepath =  working_dir+'/scratch/section/' +str(frame_nr)
    bpy.ops.render.render(write_still=True)


