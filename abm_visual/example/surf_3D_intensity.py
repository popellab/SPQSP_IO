import sys
import importlib
import bpy
from pathlib import Path

# running as a script from terminal:
path_script = Path(__file__)

# running as a script from within blender:
#path_script = Path(bpy.context.space_data.text.filepath)

#path_repo = path_script.parent.parent
path_repo = Path('D:/Research/Source/Repos/abm_visual')
sys.path.append(str(path_repo.joinpath('LIB')))

import numpy as np
import os
from scipy.spatial import Delaunay

from BlenderVisual import blender_element as be
from BlenderVisual import blender_composite as bc
from BlenderVisual import blender_utility as bu

importlib.reload(bc)
importlib.reload(be)
importlib.reload(bu)

working_dir = str(path_repo) + '/example'

delaunay_cutoff_dist = .25
cmap = {'CT':[.65,.65,0,1], 'IF': [.65,0,0,1], 'N':[0,.65,0,1]}
sample_nr = 256
subsurf_nr = 2
render_frame_nr = 20 

def process_single_file(filename, ihc_label, case_id, objectSize, z_shrink_factor):

    # load data
    header, data = bu.get_csv_data(filename, header_line = 1)
    data_3d_num = data[:,0:3].astype(float)
    # slide x/y coordinate is left-hand
    data_3d_num[:,[0,1]] = data_3d_num[:,[1,0]] 

    print(np.max(data_3d_num, 0))    
    fieldCenter = objectSize/2
  
    bu.reset_world()
    scene = bpy.data.scenes[0]

    # render
    bu.render_config(1500, 1000, sample_nr, True)
    
    # camera and lighting
    studio = bc.photo_studio(scene)
    studio.set_location(fieldCenter)
    studio.set_subject_size(25)
    studio.adjust_light('key', strength = 5000, size = 10, rot_x=np.pi/4)
    studio.adjust_light('fill', strength = 2000, rot_x=np.pi/4)
    studio.adjust_light('rim', strength = 1000)
    studio.adjust_camera(rot_x = np.pi*6.5/18)
    studio.set_rotation(0, np.pi)
    # axis
    mat_black = be.createEmissionMaterial('k_emission', [0,0,0,1])
    anchor_axis = studio.create_camera_anchor('axis_anchor', [8.5, -6, -60])
    arrow = bc.axis_arrow(scene, 2, switch_xy = True) #left-hand xy
    arrow.set_anchor(anchor_axis)
    arrow.reset_location()
    arrow.set_mat(mat_black)

    # watermark
    wm_text = 'Case {}, {}+\nx: [0, {}], y: [0, {}] mm\nz: [0, {}] mm^-2'.format(case_id, 
        ihc_label, objectSize[1], objectSize[0], objectSize[2]*z_shrink_factor)
        
    anchor_wm = studio.create_camera_anchor('wm_anchor', [-11, 7, -60])
    wm = bc.watermark(scene, wm_text)
    wm.set_anchor(anchor_wm)
    wm.set_size(.5)
    # background
    scene.world.color = (1,1,1)
    scene.world.cycles_visibility.diffuse = False

    # create vertices
    vert = np.append(data_3d_num[:,:2], np.reshape(data_3d_num[:,2]/z_shrink_factor, (-1,1)), 1)

    #points = np.random.rand(20, 2)
    points = data_3d_num[:,:2]
    tri = Delaunay(points)

    def edge_length(i, j, verts):
        return sum((verts[i]-verts[j])**2)**.5

    def remove_simplice(cutoff, s, verts):
        l1 = edge_length(s[0], s[1], verts)
        l2 = edge_length(s[1], s[2], verts)
        l3 = edge_length(s[2], s[0], verts)
        return max(l1, l2, l3) > cutoff

    faces = tri.simplices.copy()

    # remove distant triangles
    faces2 = [s for s in faces if not remove_simplice(delaunay_cutoff_dist, s, points)]

    # create mesh
    ob = be.create_Faces('surf', bpy.data.scenes['main'], vert, faces2)

    # map color to vertex ids
    colors = [cmap[x.replace('"','')] for x in data[:,-1]]
    vert_color = be.create_vertex_color('vcol', ob, colors)
    
    # set material
    mat = be.createVertexColorMaterial('vertex_mat', 'vcol')
    ob.active_material = mat

    # set smooth
    for pol in ob.data.polygons:
        pol.use_smooth = True
        
    # subsurf
    subsurf = ob.modifiers.new(name = 'subsurf', type = 'SUBSURF')
    subsurf.render_levels = subsurf_nr


    # box
    box = be.visual_box('box', fieldCenter, objectSize, 0.01, (1,.5,0,1))

    #############################
    # render setting
    #############################

    # turn
    def rotation(scene):
        f = scene.frame_current
        f0 = scene.frame_start
        f1 = scene.frame_end
        studio.set_rotation(0, np.interp(f, [f0, f1], [-np.pi/4, np.pi*(1+3/4)]))
        scene.view_layers[0].update()# for children object to transform   
        arrow.reset_location()
        return

    scene.frame_start = 0
    scene.frame_end = render_frame_nr
    render_dir = working_dir + '/scratch/density/Case{}/'.format(case_id)

    frames = list(range(scene.frame_end+1))

    for frame_nr in frames:
        scene.frame_set(frame_nr)
        rotation(scene)
        # set output path so render won't get overwritten
        img_filename = '{}_Case{}_{}'.format( ihc_label, case_id, str(frame_nr))
        scene.render.filepath = render_dir + img_filename
        bpy.ops.render.render(write_still=True)
    

#############################
# batch visualization
#############################
    
case_id = 1
ihc_label = 'CD8'
z_shrink_factor = 2500
objectSize = np.asarray([18, 32, 4])

filename = working_dir + '/data/density/3D_data_{0}_Case{1}.csv'.format(ihc_label, case_id)

process_single_file(filename, ihc_label, case_id, objectSize, z_shrink_factor)

"""
labels = ['CD3', 'CD4', 'CD8', 'CD20', 'FoxP3']
cases = [1, 2, 3, 4, 5]
sizes = {1: np.array([22, 28, 4]), 
         2: np.array([18, 32, 4]), 
         3: np.array([20, 28, 4]), 
         4: np.array([24, 30, 4]), 
         5: np.array([18, 22, 4])} #[y, x, z]
         
shrink = {'CD3': {1: 2500, 2: 2500, 3: 2500, 4: 1500, 5: 2000},
          'CD4': {1: 2500, 2: 2500, 3: 2500, 4: 1500, 5: 2000},
          'CD8': {1: 2500, 2: 2500, 3: 2500, 4: 1500, 5: 2000},
          'CD20': {1: 2500, 2: 2500, 3: 2500, 4: 1500, 5: 2000},
          'FoxP3': {1: 2500, 2: 500, 3: 2500, 4: 1500, 5: 2000}}
for case_id in cases:
    for ihc_label in labels:
        filename = os.path.join( working_dir, 
            r'3D_data_Case{1}/3D_data_{0}_Case{1}.csv'.format(ihc_label, case_id))
        process_single_file(filename, ihc_label, case_id, sizes[case_id], shrink[ihc_label][case_id])        
"""