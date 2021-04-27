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
bu.render_config(x=800, y=800, sample=32, use_gpu = False)

world = bu.reset_world()
world.color = (0.05,0.05,0.05)
#world.color = (1,1,1)
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
studio.set_subject_size(100)
studio.set_rotation(0, np.pi/8*7)
studio.adjust_light('key', strength = 4e5, rot_x=np.pi/3)
studio.adjust_light('fill', strength = 1e5, rot_x=np.pi/3)
studio.adjust_light('rim', strength = 5e5, rot_x=np.pi/3)

mat_white = be.createEmissionMaterial('w_emission', [1,1,1,1])

anchor_axis = studio.create_camera_anchor('axis_anchor', [9, -10, -60])
arrow = bc.axis_arrow(sce, 2) 
arrow.set_anchor(anchor_axis)
arrow.reset_location()
#arrow.set_mat(mat_white)
    
wm_text = 'ABM simulated IHC'
anchor_wm = studio.create_camera_anchor('wm_anchor', [-10, 10, -60])
wm = bc.watermark(sce, wm_text, mat_white)
wm.set_anchor(anchor_wm)
    


################################
# objects
################################

from PIL import Image, ImageDraw
img_dim = (1000, 1000)
image = Image.new('RGB', img_dim, 'white')
draw = ImageDraw.Draw(image)

voxel_nr = 100
voxel_size = 20
ppv = img_dim[0]/voxel_nr
# pixel per micron
ppm = ppv/voxel_size

# radius map from cell type. Unit: microns
r_map = {1:10, 2:5}

# return staining type id for a cell info array
# high priority stainings have larger ID values
def cell_to_stain_id(cell):
    stain_id = 0
    if cell[4] == 1:
        stain_id = 1
    return stain_id

# mapping stain id to color rgba
c_map = {0: [.4,.6,.8], 1: [.3,.1,.15]}

def jiggleCell(c):
    r = np.random.rand(*c.shape)*.5
    c = c+r
    return c


def draw_y_slice(y_slice):
    dy = np.abs(coords[:,1] - y_slice)
    in_y_slice = dy < 0.5
    # paint white background
    image.paste( (255,255,255),[0,0,image.size[0],image.size[1]])
    # row number for cell in slice
    idx = np.array([i for i, test in enumerate(in_y_slice) if test])
    if len(idx) > 0:
        cells = np.zeros((len(idx), 6))
        # convert coordinates to pixels
        coords_pixel_slice = coords[idx,:]*ppv
        # y slices coordinates should be in the order of (z, x)
        cells[:,[0,1]] = coords_pixel_slice[:, [2,0]]
        # radius to pixel
        cells[:,2] = np.array([r_map[celldata[i, 3]] for i in idx])*ppm
        stain_value = np.array([cell_to_stain_id(c) for c in celldata[idx]])
        c_color = np.array([c_map[s] for s in stain_value])
        cells[:,3:] = c_color
        # cell: list of cells to draw. [x, y, radius, r, g, b]
        srt_idx = np.argsort(stain_value) # sort high priority to the end so that their color on top
        bu.draw_IHC(draw, cells[srt_idx,:])
    rgb = np.array(image)
    rgba = np.pad(rgb, ((0,0),(0,0),(0,1)), mode='constant', constant_values=255)/255
    img = be.createImage('ihc', rgba)
    return img

vol = bc.box_slice('vol', sce, objectSize, (1,.3,0,1))
s1 = vol.add_slice('y', 1, 0)
coords = jiggleCell(celldata[:,:3])

def visualSlice(scene):    
    f = scene.frame_current
    f0 = scene.frame_start
    f1 = scene.frame_end
    y_min = .5
    y_max = 50.5
    y_slice = np.interp(f, [f0, f1], [y_min, y_max])
    img = draw_y_slice(y_slice)
    vol.update_plane(s1, img, 1, y_slice)
    wm.set_text('y = {:.2f}mm'.format(y_slice*0.02))

################################
# Visualized data
################################



sce.frame_start = 0
sce.frame_end = 25
sce.frame_current = 0

# movie
bpy.app.handlers.frame_change_pre.clear()
bpy.app.handlers.frame_change_pre.append(visualSlice)

sce.render.image_settings.file_format = 'JPEG'


frames = list(range(sce.frame_end+1))
for frame_nr in frames:
    # set current frame to frame 5
        
    sce.frame_set(frame_nr)
    sce.render.filepath = working_dir+'/scratch/ihc/'+ str(frame_nr)
    bpy.ops.render.render(write_still=True) # render still
