"""
BlenderVisual.blender_composite

Composite visualization units as classes

Created on Tue Jun 25 11:39:49 2019
@author: Chang Gong

"""

import bpy
import numpy as np
from . import blender_element as be
from scipy.spatial import Delaunay
###############################################################################
## Studio setup
###############################################################################
## Three-point lighting and 80mm focal length camera
## Default subject size: 100
class photo_studio():
    def __init__(self, scene):
        self._scene = scene
        # center point
        self._stage_center = bpy.data.objects.new('studio_center', None)
        scene.collection.objects.link(self._stage_center)
        # setup camera
        self._cam = self._setup_camera()
        self.adjust_item(self._cam, rot_x = np.pi*5/12, rot_z=0)
        # list of camera anchors
        self._cam_anchor = []
        # setup 3-point lighting
        self._lights = {}
        self._lights['key'] = self._setup_light_key(0, 1)
        self._lights['fill'] = self._setup_light_fill(0, 1)
        self._lights['rim'] = self._setup_light_rim(0, 1)
        self.adjust_light('key', rot_x=np.pi*5/12, rot_z=np.pi/4)
        self.adjust_light('fill', rot_x=np.pi*5/12, rot_z=-np.pi/4)
        self.adjust_light('rim', rot_x=np.pi/4, rot_z=np.pi)
        self.set_subject_size(100)
        return
    def get_lights(self):
        return self._lights
    def get_camera(self):
        return self._cam
    # attach a pivit point to a studio obj (e.g. light)
    # and set stage center as parent to pivot
    def _attach_pivot(self, obj):
        pivot = bpy.data.objects.new(obj.name + '_pivot', None)        
        self._scene.collection.objects.link(pivot)
        obj.parent = pivot
        pivot.parent = self._stage_center
        return
    def _setup_camera(self):
         # remove original cameras
        cams = bpy.data.cameras
        for c in cams:
            cams.remove(c, do_unlink = True)
        if bpy.data.objects.get('Camera'):
            bpy.data.objects.remove(bpy.data.objects['Camera'])
        # create camera and camera settings
        cam_settings = bpy.data.cameras.new('CamSetting')
        cam_settings.lens = 80
        cam = bpy.data.objects.new('Camera', cam_settings)
        # link camera to scene
        self._scene.collection.objects.link(cam)
        self._attach_pivot(cam)
        self._scene.camera = cam        
        return cam    
    def _setup_light_key(self, strength, size):
        light = be.addAreaLight('key_area', strength, (1,1,1,1), size, self._scene)
        self._attach_pivot(light)
        return light
    def _setup_light_fill(self, strength, size):
        light = be.addAreaLight('fill_area', strength,  (1,1,1,1), size, self._scene)
        self._attach_pivot(light)
        return light
    def _setup_light_rim(self, strength, size):
        light = be.addAreaLight('rim_area', strength, (1,1,1,1), size, self._scene)
        self._attach_pivot(light)
        return light    
    def adjust_light(self, light_key, dist=None, strength=None, size = None,
                     rot_x=None, rot_z = None):
        light = self._lights[light_key] 
        if strength is not None:
            light.data.node_tree.nodes['Emission'].inputs['Strength'].default_value = strength
        if size is not None:
            light.scale = (size, size, 1)
        self.adjust_item(light, dist, rot_x, rot_z)
        return
    def adjust_camera(self, dist=None, rot_x=None, rot_z = None):
        self.adjust_item(self._cam, dist, rot_x, rot_z)
        return     
    def adjust_item(self, obj, dist=None, rot_x=None, rot_z = None):
        if dist is not None:
            obj.location[2] = dist
        if rot_x is not None:
            obj.parent.rotation_euler.x = rot_x
        if rot_z is not None:
            obj.parent.rotation_euler.z = rot_z
        return 
    # set location of studio center
    def set_location(self, crd):
        self._stage_center.location = crd
        return
    # set rotation of studio: first along x by x_rot, 
    # then z by z_rot. both are rads. (right hand rotation)
    def set_rotation(self, rot_x, rot_z):
        self._stage_center.rotation_euler.x = rot_x
        self._stage_center.rotation_euler.z = rot_z
        return
    # scale lighting and camera location/strength based on subject size
    def set_subject_size(self, size):
        # default size is 100
        s = size/100
        self.adjust_item(self._cam, dist = 400*s)
        self._cam.data.clip_end = max(1000 * s, 100)
        self.adjust_light('key', dist = 200*s, strength = 4e4*s**2, size = 10*s)
        self.adjust_light('fill', dist = 200*s, strength = 2e4*s**2, size = 40*s)
        self.adjust_light('rim', dist = 200*s,strength = 5e4*s**2, size = 10*s)
        return
    # create an empty location fixed to camera perspective
    def create_camera_anchor(self, name, crd):
        anchor = bpy.data.objects.new(name, None)
        anchor.location = crd
        anchor.parent = self._cam
        self._scene.collection.objects.link(anchor)
        self._cam_anchor.append(anchor)
        return anchor 
        
###############################################################################
## Gadgets 
###############################################################################

# gadgets are anchored to camera. self.head is a member object (empty point),
# which is parent of all class objects directly or indirectly
# Use self.head for scaling.
class gadget_base():
    def set_scale(self, scale):
        self.head.scale[0]=self.head.scale[1]=self.head.scale[2]=scale
        return

## create xyz axis from origin with length l.
#   emission material, rgb correspond to xyz 
class axis_arrow(gadget_base):
    def __init__(self, scene, length, switch_xy = False, label=None):
        self._length = length
        self._scene = scene
        self._red = be.createEmissionMaterial('arrow_r_emmision', [1,0,0,1])
        self._green = be.createEmissionMaterial('arrow_g_emmision', [0,1,0,1])
        self._blue = be.createEmissionMaterial('arrow_b_emmision', [0,0,1,1]) 
        self._arrow = bpy.data.objects.new('axis_arrows', None)
        self.head = self._arrow
        scene.collection.objects.link(self._arrow)
        self._x = bpy.data.objects.new('x_tip', None)
        self._y = bpy.data.objects.new('y_tip', None)
        self._z = bpy.data.objects.new('z_tip', None)
        scene.collection.objects.link(self._x)
        scene.collection.objects.link(self._y)
        scene.collection.objects.link(self._z)
        self._x.parent = self._arrow
        self._y.parent = self._arrow
        self._z.parent = self._arrow
        self._texts = []
        if label is None:
            label = ['x','y','z']
        if switch_xy:
            label[0], label[1] = label[1], label[0] 
        self._x_axis = self._create_arrow(self._red, 0, label[0], self._x)
        self._y_axis = self._create_arrow(self._green, 1, label[1], self._y)
        self._z_axis = self._create_arrow(self._blue, 2, label[2], self._z)
        return
    def _create_arrow(self, mat, dim, axis_name, tip):
        l = self._length
        thickness = l/20
        tip_fraction = 1/5
        tip_length = l*tip_fraction
        stem_length = l*(1-tip_fraction)
        tip_center = l*(1-1/2*tip_fraction)
        stem_center = l*(1-tip_fraction)/2
        rotations = [[0,np.pi/2,0], [-np.pi/2,0,0], [0,0,0]] 
        # arrow head
        bpy.ops.mesh.primitive_cone_add(radius1=thickness,depth=tip_length)
        ob_head = bpy.context.object
        ob_head.name = 'arrow_head'+axis_name
        ob_head.location = [0, 0, 0]
        ob_head.location[dim] = tip_center
        ob_head.rotation_euler = rotations[dim]
        ob_head.parent = self._arrow
        ob_head.active_material = mat
        # arrow stem
        bpy.ops.mesh.primitive_cylinder_add(radius = thickness/2.5, depth=stem_length)
        ob_stem = bpy.context.object
        ob_stem.name = 'arrow_stem'+axis_name
        ob_stem.location = [0, 0, 0]
        ob_stem.location[dim] = stem_center
        ob_stem.rotation_euler = rotations[dim]
        ob_stem.parent = self._arrow
        ob_stem.active_material = mat      
        # text
        text = bpy.data.curves.new(type="FONT",name="arrow_text"+axis_name)
        textob = bpy.data.objects.new("arrow_text"+axis_name,text)
        textob.data.body = axis_name
        textob.scale[0]=textob.scale[1]=(l/2)
        tip.location[dim] = l*1.2
        textob.active_material = mat
        self._texts.append(textob)
        self._scene.collection.objects.link(textob)
        textob.parent = self._arrow
        return [ob_head, ob_stem, textob]
    # set text parent to camera anchor to lock orientation
    def set_anchor(self, anchor):
        self._anchor = anchor
        #for t in self._texts:
        #    t.parent = anchor
        return
    # because set arrow to anchor location and rotate text to face camera
    def reset_location(self):
        # self._scene.update()
        layer = bpy.context.view_layer
        layer.update()
        #loc = np.asarray(self._anchor.matrix_world)[:3,3]
        loc = self._anchor.matrix_world.to_translation()
        self._arrow.location = loc
        rot = self._anchor.matrix_world.to_euler('XYZ')
        self._texts[0].location = self._x.location
        self._texts[0].rotation_euler = rot
        self._texts[1].location = self._y.location
        self._texts[1].rotation_euler = rot
        self._texts[2].location = self._z.location
        self._texts[2].rotation_euler = rot
        return
    # default: rgb emission
    def set_mat(self, mat=None):
        if mat is None:
            for o in self._x_axis:
                o.active_material = self._red
            for o in self._y_axis:
                o.active_material = self._green
            for o in self._z_axis:
                o.active_material = self._blue
        else:
            for o in self._x_axis:
                o.active_material = mat
            for o in self._y_axis:
                o.active_material = mat
            for o in self._z_axis:
                o.active_material = mat
        return

# watermark for image
class watermark(gadget_base):
    def __init__(self, scene, text, mat_text=None):
        wm_curv = bpy.data.curves.new(type="FONT",name="watermark_curv")
        wm_obj = bpy.data.objects.new("watermarkText",wm_curv)
        wm_obj.data.body = text
        scene.collection.objects.link(wm_obj)
        if mat_text is None:
            wm_obj.active_material = be.createEmissionMaterial('k_emmision', [0,0,0,1])
        else:
            wm_obj.active_material = mat_text
        self._wm_obj = wm_obj
        return
    def set_text(self, text):
        self._wm_obj.data.body = text
        return
    def set_anchor(self, anchor):
        self._wm_obj.parent = anchor
        self.head = anchor
    def set_size(self, s):
        self._wm_obj.scale = (s, s, 1)
        return

# colorbar        
class colorbar(gadget_base):
    def __init__(self, scene, colormap, smin, smax, use_log = False):
        self._mat_txt = be.createEmissionMaterial('black', [0,0,0,1])
        self._use_log = use_log
        self._max = smax
        self._min = smin
        self._ticks = []
        self._tickobs = []
        self._scene = scene
        # bar
        nr_colors = 200
        bar_length = 10
        bar_width = 1 
        self._barsize = [bar_length, bar_width]
        if use_log:
            bar_values = 10 ** (np.interp(np.arange(nr_colors+1), [0, nr_colors], [np.log10(smin), np.log10(smax)]))
        else:
            bar_values = np.interp(np.arange(nr_colors+1), [0, nr_colors], [smin, smax])
        bar_tile = np.tile(bar_values, [1,1])
        color_bar_center=bpy.data.objects.new('colorbar', None)
        color_bar_center.location = [0,0,0]
        scene.collection.objects.link(color_bar_center)
        rgba = be.mapToColor(bar_tile, colormap, xmin=smin, xmax= smax,log=use_log, maptype='RGBA')
        bar_img = be.createImage('color_bar', rgba)
        bar_mat = be.createPlaneTextureFromImage('bar_mat', bar_img)
        
        colorbar = be.createImagePlane('colorbar_colors', [bar_width,bar_length,0], bar_mat)
        colorbar.location=[0,0,0]
        colorbar.active_material.node_tree.nodes['Image Texture'].interpolation='Closest'
        #colorbar visibility
        #colorbar.cycles_visibility.camera = False
        colorbar.cycles_visibility.diffuse = False
        colorbar.cycles_visibility.glossy = False
        colorbar.cycles_visibility.transmission = False
        colorbar.cycles_visibility.scatter = False
        colorbar.cycles_visibility.shadow = False

        self._bar = colorbar
        colorbar.parent = color_bar_center
        self._center = color_bar_center
        return
    def set_brightness(self, x):
        self._bar.active_material.node_tree.nodes['Emission'].inputs[1].default_value = x
        return
    def annotate(self, nr_tick, title, tick_format = '.2e'):
        font_size = .5
        if self._use_log:
            tick_values = 10 ** (np.interp(np.arange(nr_tick), [0, nr_tick-1], [np.log10(self._min), np.log10(self._max)]))
        else:
            tick_values = np.interp(np.arange(nr_tick), [0, nr_tick-1], [self._min, self._max])

        tick_loc = np.interp(np.arange(nr_tick), [0, nr_tick-1], 
                [0, self._barsize[0]]) - self._barsize[0]/2
        for i, v in enumerate(tick_values):
            txt = ('__{:'+tick_format+'}').format(v)
            loc = [self._barsize[1]*.5-font_size*.5, tick_loc[i]+font_size*.1, .001]
            self._add_tick(loc, txt, font_size)
        loc_title = [-self._barsize[1]/2, tick_loc[-1]+font_size/2, 0]
        self._add_tick(loc_title, title, font_size)
        return
    def _add_tick(self, loc, txt, fs):
        tick=bpy.data.objects.new('tick', None)
        tick.location = loc
        self._scene.collection.objects.link(tick)
        self._ticks.append(tick)
        tick.parent = self._center
        text = bpy.data.curves.new(type="FONT",name="color_tick")
        textob = bpy.data.objects.new("color_tick",text)
        self._tickobs.append(textob)
        textob.data.body = txt
        textob.data.size = fs
        textob.active_material = self._mat_txt
        self._scene.collection.objects.link(textob)
        textob.parent = tick
        return
    def update_range(self, smin, smax, tick_format = '.2e'):
        nr_tick = len(self._ticks) - 1
        self._max = smax
        self._min = smin
        if self._use_log:
            tick_values = 10 ** (np.interp(np.arange(nr_tick), [0, nr_tick-1], [np.log10(self._min), np.log10(self._max)]))
        else:
            tick_values = np.interp(np.arange(nr_tick), [0, nr_tick-1], [self._min, self._max])
        for i, v in enumerate(tick_values):
            txt = ('__{:'+tick_format+'}').format(v)
            self._tickobs[i].data.body = txt 
        return

    def set_anchor(self, anchor):
        self._center.parent = anchor
        self.head = anchor
        return
    def resize_text(self, s):
        for t in self._ticks:
            t.scale[0] = t.scale[1] = s
        return

###############################################################################
## ABM agents: cell
###############################################################################
class cell_base():
    def __init__(self, name, scene, proto, crd):
        self._scene = scene
        seed = 0
        lifetime = 10000
        cell = be.create_Vertices(name, scene, crd)
        #bpy.context.scene.objects.active = cell
        cell.select_set(True)
        cell.modifiers.new("cell_particle", type='PARTICLE_SYSTEM')
        part = cell.particle_systems[0]
        part.seed = seed
        settings = part.settings
        settings.type = 'EMITTER'
        settings.count = len(crd)
        settings.frame_start = 0
        settings.frame_end = 0
        settings.lifetime = lifetime
        settings.emit_from = 'VERT'
        settings.use_emit_random = False
        settings.render_type = 'OBJECT'
        settings.instance_object = proto
        settings.physics_type = 'NO'
        settings.particle_size = 1
        settings.use_rotations = True
        settings.rotation_mode = 'VEL'
        settings.rotation_factor_random = 2
        settings.phase_factor_random = 2
        self._cell = cell
        self._proto = proto
        #instantiate particles
        self._cell.users_scene[0].view_layers[0].update()
        return
    def update_cells(self, crd):
        me = bpy.data.meshes.new('testMesh')
        me.from_pydata(list(crd), [], [])
        me_old = self._cell.data
        self._cell.data = me
        self._cell.particle_systems[0].settings.count = len(crd)
        self._cell.users_scene[0].view_layers[0].update()
        bpy.data.meshes.remove(me_old)
        #self._scene.view_layers[0].update()
        return
    def get_cell(self):
        return self._cell
    def get_proto(self):
        return self._proto
    def set_render(self, preview, render):
        self._cell.hide_viewport = not preview
        self._cell.hide_render = not render
        return
    # particle emission can sometimes fail to follow the exact sequence
    # by replicating or missing vertices.
    # return the row numbers in vertices. correspond to each particle
    def _getParticleIdxCorrection(self):
        id_map = {}
        v2_80_bug_fix = False 
        if v2_80_bug_fix:
            # currently bugged. returns an empty collection
            part = self._cell.particle_systems[0].particles
        else: 
            # for this workaround to work, object needs to be visible from viewport 
            # and not just rendering engine.
            dg = bpy.context.evaluated_depsgraph_get()
            ob = self._cell.evaluated_get(dg)
            part = ob.particle_systems.active.particles
        vert = self._cell.data.vertices
        for i, v in enumerate(vert):
            id_map[(v.co[0],v.co[1],v.co[2])] = i
        part_seq = [id_map[(p.location[0],p.location[1],p.location[2])] for p in part]
        return np.array(part_seq).astype(int)

# cells with the same color
class cell_mono(cell_base):
    def __init__(self, name, scene, proto, crd, color):
        cell_base.__init__(self, name, scene, proto, crd)
        proto.active_material = be.createDiffussionSurfaceMaterial('mat_'+name, color)
        return
    
# cells with each of their color explicitly determined
class cell_color(cell_base):
    def __init__(self, name, scene, proto, crd, colors):
        cell_base.__init__(self, name, scene, proto, crd)
        particle_vert_id = self._getParticleIdxCorrection()
        colors_corrected = colors[particle_vert_id]
        #colors_corrected = colors
        proto.active_material = self._mat = be.createColorMappedParticleMaterial(colors_corrected)
        return
    # update cell locations and colors
    def update_cells(self, crd, colors):
        cell_base.update_cells(self, crd)
        particle_vert_id = self._getParticleIdxCorrection()
        colors_corrected = colors[particle_vert_id]
        #colors_corrected = colors
        be.updateParticleColor(self._mat, colors_corrected)
        return

###############################################################################
## ABM agents: plane
###############################################################################
# a slice in volume bound by a box
class box_slice():
    def __init__(self, name, scene, dim, color_box):
        self._volume = bpy.data.objects.new(name, None)
        scene.collection.objects.link(self._volume)
        self._scene = scene
        self._center = dim/2
        self._dim = dim
        self._box = be.visual_box('box_'+name, self._center, dim, 0.01, color_box)
        self._planes = set()
        self._box.parent = self._volume
        self._divider = None
        return
    # axis: perpendicular to. take {0,1,2}
    # loc: location of cut on axis.
    # img_in(optional): initial image
    def add_slice(self, name, axis, loc, transparent=False, img=None):
        self._rot = {0: (np.pi/2, 0, np.pi/2), 
                     1: (0, -np.pi/2, -np.pi/2), 
                     2: (0,0,0)}
        rgbValues = np.zeros((10, 10, 4))
        for i in range(10):
            for j in range(10):
                rgbValues[i, j, 0]=i/10
                rgbValues[i, j, 1]=j/10
                rgbValues[i, j, 3]=1
        img_default = be.createImage('default', rgbValues)
        if transparent:
            imgMat = be.createTransparentTextureFromImage('mat_'+name, img_default)
        else:
            imgMat = be.createPlaneTextureFromImage('mat_'+name, img_default)
        dim = [self._dim[(axis+1)%3], self._dim[(axis+2)%3], 0]
        plane = be.createImagePlane('plane_'+name, dim, imgMat)
        plane.location = self._center
        plane.location[axis] = loc
        plane.rotation_euler = self._rot[axis]
        self._planes.add(plane)
        plane.parent = self._volume
        if img is not None:
            self.update_plane(plane, img, axis, loc)
        return plane
    # adjust strength of emission node
    def set_brightness(self, plane, x):
        if plane in self._planes:
            emission = plane.active_material.node_tree.nodes['Emission']
            emission.inputs[1].default_value = x
        return
    # update plane image and location
    # image coordinates: x cut: yz; y cut: zx; z cut: xy
    def update_plane(self, plane, img, axis, loc):
        if plane in self._planes:
            be.updateMaterialImage(plane.active_material, img)
            plane.location[axis] = loc
            if self._divider:
                for i in [0,1,2]:
                    if i == axis:
                        continue
                    else:
                        self._divider_obs[i].location[axis] = loc
                
        return
    # add divider for planes intersection
    def set_divider(self, cross, radius, color = (1,1,1,1)):
        if self._divider is None:
            self._divider=bpy.data.objects.new('divider', None)
            self._scene.collection.objects.link(self._divider)
            self._divider_obs = []
            mat = be.createEmissionMaterial('divider_emmision', color)
            for axis in [0,1,2]:
                bar = self._create_plane_divider('bar_{}'.format(axis), self._dim[axis], axis, radius, mat)
                bar.parent = self._divider
                self._divider_obs.append(bar)
        obs = self._divider_obs
        obs[0].location[1] = obs[2].location[1] = cross[1]
        obs[0].location[2] = obs[1].location[2] = cross[2]
        obs[1].location[0] = obs[2].location[0] = cross[0]
        return
    
    # divider when creating xyz cross sections in one figure
    def _create_plane_divider(self, name, l, axis, radius, mat):
        bpy.ops.mesh.primitive_cylinder_add(radius = radius, depth=l)
        ob = bpy.context.object
        ob.name = name
        ob.active_material = mat
        ob.location[axis] = self._center[axis]
        ob.rotation_euler[(1-axis)%3] = np.pi/2
        return ob
###############################################################################
## ABM agents: network
###############################################################################        
## visualize a graph
# this version create a curve from vertex/edge mesh,
# use a circle to bevel the curve, and create individual material for each
# spline by mapping curve uv to a color pixel array image
# limitations:
#   1. if graph contains lots of splines, generating new material datablock become
#      increasingly slow, and fails at certain point (>32000).
#   2. slightly slower than the skin method
# Use this unless the number of splines is large and color mapping is requred
class network_curve_base():
    def __init__(self, name, scene, vert, edge, diameter):
        # map vertex id pairto edge index
        self._edge_map = {self._vert_pair_key(min(e), max(e)): i for i, e in enumerate(edge)}
        # same bevelObjects for all
        bpy.ops.curve.primitive_bezier_circle_add(radius=.5)
        bevelObj = bpy.context.object
        bevelObj.name = 'graphBevel'
        bevelObj.data.resolution_u = 5
        bevelObj.data.render_resolution_u = 0
        scene.collection.objects.unlink(bevelObj)
        self._diameter_set = False
        ## get spline lists: create a dummy curve    
        nrV = vert.shape[0]
        vIdx = np.arange(nrV)
        blankYZ = np.zeros(nrV)
        # x of vDummy is vertex index
        vDummy = list(np.transpose(np.stack((vIdx, blankYZ, blankYZ))))
        me = bpy.data.meshes.new('DummyMesh')
        me.from_pydata(vertices=vDummy, edges=list(edge), faces=[])
        ob = bpy.data.objects.new('DummyObj', me)
        scene.collection.objects.link(ob)
        scene.view_layers[0].objects.active = ob
        ob.select_set(True)
        bpy.ops.object.convert(target='CURVE')
        # [[s0_v0, s0_v1, ...], [[s1_v0, s1_v1, ...],...]
        self._spline_vert_list =  [[int(p.co[0]) for p in s.points] for s in ob.data.splines]
        bpy.data.objects.remove(ob)
        
        # [[s0_e0, s0_e1, ...], [[s1_e0, s1_e1, ...],...]
        self._spline_edge_list = [[self._edge_map[self._vert_pair_key(spline[i], 
            spline[i+1])] for i in range(len(spline)-1)] for spline in self._spline_vert_list]
    
        # create mesh and curve: 
        fullGraphName = name
        curveFull = be.creaetCurveFromList(fullGraphName, vert, self._spline_vert_list)
        self._network = curveFull_obj = bpy.data.objects.new(name=fullGraphName, object_data=curveFull)
        self._network.data.bevel_object = bevelObj
        scene.collection.objects.link(curveFull_obj)
        self._bevel = bevelObj
        self.set_diameter(diameter)
        return
    # generate a key from two integer vertex indices
    def _vert_pair_key(self, v0, v1):
        return (min(v0, v1), max(v0, v1))
    # set diameters. diameter: scalar or list of size n
    def set_diameter(self, diameter):
        if hasattr(diameter, "__len__"):
            self._diameter_set = True
            self._bevel.scale[0] = self._bevel.scale[1] = 1
            for i, spline in enumerate(self._spline_vert_list):
                for j, p in enumerate(spline):
                    # It's blender's fault that radius values actually determine diameters.
                    self._network.data.splines[i].points[j].radius = diameter[p]
        else:
            self._bevel.scale[0] = self._bevel.scale[1] = diameter
            if self._diameter_set:
                for i, spline in enumerate(self._spline_vert_list):
                    for j, p in enumerate(spline):
                        self._network.data.splines[i].points[j].radius = 1
                self._diameter_set = False
        return
    # set bevel resolution. default is 0 for render and 1 for preview
    def set_bevel_resolution(self, preview, render):
        self._bevel.data.resolution_u = preview
        self._bevel.data.render_resolution_u = render
        return 
    def set_render(self, preview, render):
        self._network.hide = not preview
        self._network.hide_render = not render
        return

# network with the same color
# name: name of the object
# scene: scene where object is placed on
# vert: vertex coords (n by 3)
# edge: edges (m by 2)
# color: single rgba (1 by 4)
class network_curve_mono(network_curve_base):
    def __init__(self, name, scene, vert, edge, diameter, color):
        network_curve_base.__init__(self, name, scene, vert, edge, diameter)
        mat = be.createDiffussionSurfaceMaterial('mat' + name, color)
        self._network.active_material = mat
        return
    
# network with color mapped to each vertex
# name: name of the object
# scene: scene where object is placed on
# vert: vertex coords (n by 3)
# edge: edges (m by 2)
# colormap: x by 4 array
# colors_fac: rgba (n, [0,1])
class network_curve_color(network_curve_base):
    def __init__(self, name, scene, vert, edge, diameter, colormap, colors_fac):
        network_curve_base.__init__(self, name, scene, vert, edge, diameter)
        color_edge = be.vertToEdgeAverage(edge, vert, colors_fac)
        color_edge_rgba = be.mapToColor(color_edge, colormap, xmin = 0, xmax = 1, maptype='RGBA')
        self._network.data.use_uv_as_generated = True
        # set up material
        for i, spline in enumerate(self._network.data.splines):
            mat=be.createColorMappedEdgeMaterial(color_edge_rgba[self._spline_edge_list[i]], 
                                                 name = name+'_mat_{}'.format(i))
            self._network.data.materials.append(mat)
            spline.material_index = i
        return

## use mesh as bone and skin modifier to visualize graphs.
# in this version, 
# limitations:
#   1. Not as accurate as curve-bevel method in visualizing tube curvatures
import time        
class network_skin_base():
    def __init__(self, name, scene, vert, edge, diameter):
        start = time.time()
        self._network = obj = be.create_Edges(name, scene, vert, edge)
        skin = obj.modifiers.new("skin", type='SKIN')
        skin.use_x_symmetry = False
        skin.use_y_symmetry = False
        skin.use_z_symmetry = False
        for i, vert in enumerate(obj.data.skin_vertices[0].data):
            vert.radius[0]=vert.radius[1] = diameter[i]/2
            vert.use_root = True
        deci = obj.modifiers.new("decimate", type='DECIMATE')
        deci.decimate_type = 'DISSOLVE'
        # decimate_rad_limit: angle limit for decimation, unit: rad
        decimate_rad_limit = 0.30
        deci.angle_limit = decimate_rad_limit
        deci.use_dissolve_boundaries = True
        print('modifiers: {}'.format(time.time()-start))
        return
    # smoothen surface. 
    def _smoothen(self, subsurf):
        bpy.context.scene.view_layers[0].objects.active =  self._network
        self._network.select_set(True)
        for pol in self._network.data.polygons:
            pol.use_smooth = True    
        if subsurf > 0:
            sub = self._network.modifiers.new("subsurf", type='SUBSURF')
            sub.render_levels=subsurf
            #sub.use_subsurf_uv = False
        bpy.ops.object.convert(target='MESH')
        return
    
    def set_render(self, preview, render):
        self._network.hide = not preview
        self._network.hide_render = not render
        return

# name: name of the object
# scene: scene where object is placed on
# v: vertex coords (n by 3)
# e: edges (m by 2)
# diameter: (1 or n)
# color: single rgba (1 by 4)
# subsurf: subsurf level. Do not add subsurface modifier if 0.
class network_skin_mono(network_skin_base):
    def __init__(self, name, scene, vert, edge, diameter, color, subsurf = 2):
        network_skin_base.__init__(self, name, scene, vert, edge, diameter)
        self._network.active_material = be.createDiffussionSurfaceMaterial(name+'_mat', color)
        self._smoothen(subsurf)
        return

# name: name of the object
# scene: scene where object is placed on
# vert: vertex coords (n by 3)
# edge: edges (m by 2)
# diameter: (1 or n)
# colormap: (4 by k), rgba of equally spaced elements in colorramp.    
# color_fac values between [0, 1] (n) 
# subsurf: subsurf level. Do not add subsurface modifier if 0.
class network_skin_color(network_skin_base):
    def __init__(self, name, scene, vert, edge, diameter, colormap, color_fac, subsurf = 2):
        network_skin_base.__init__(self, name, scene, vert, edge, diameter)
        self._map_color(name, scene, colormap, color_fac)
        #self._smoothen(subsurf)
        return
    
    def _map_color(self, name, scene, colormap, color_fac):
        start = time.time()
        obj = self._network
         # set vertex group weight
        vg = obj.vertex_groups.new(name='color')
        for i, d  in enumerate(color_fac):
            vg.add([i], d, 'REPLACE');
        # apply skin modifyer
        bpy.context.scene.view_layers[0].objects.active = obj
        obj.select_set(True)
        bpy.ops.object.convert(target='MESH') 
        """
        # this new version of to_mesh in 2.80 does not populate vertex group weight to new mesh.
        old_mesh = obj.data
        dg = bpy.context.evaluated_depsgraph_get()
        eval_obj = bpy.context.object.evaluated_get(dg)
        eval_obj.to_mesh(preserve_all_data_layers=True, depsgraph = dg) #in case we are in edit mode
        new_mesh = bpy.data.meshes.new_from_object(eval_obj)
        obj.modifiers.clear()
        obj.data = new_mesh
        bpy.data.meshes.remove(old_mesh)
        """
        print('apply: {}'.format(time.time()-start))
        # set UV
        uv = obj.data.uv_layers.new()
        be.zeroUV(obj)
        # set UV warp
        uv_warp = obj.modifiers.new("uvwarp", type='UV_WARP')
        e0=bpy.data.objects.new('empty0', None)
        scene.collection.objects.link(e0)
        e0.location = [0,0,0] 
        e1=bpy.data.objects.new('empty1', None)
        scene.collection.objects.link(e1)
        e1.location = [1,0,0]
        uv_warp.object_from = e1
        uv_warp.object_to = e0
        uv_warp.vertex_group = vg.name
        uv_warp.uv_layer = uv.name
        # material
        mat = be.createColorRampEdgeMaterial(colormap, name = name+'_skin_colorramp', interpolation = 'LINEAR')
        obj.active_material = mat
        print('material: {}'.format(time.time()-start))
        return

#############################
# 3D surface 
#############################
class surface_base():
    def __init__(self, name, scene):
        self._surf = surf = be.create_Faces(name, scene, [], [])
        return

# surface with the same color painted to each vertex 
class surface_mono(surface_base):
    def __init__(self, name, scene, mat):
        surface_base.__init__(self, name, scene)
        self._mat = mat
        return
    def swap_surface(self, verts, simplices):
        ob = self._surf
        be.swap_face(ob, verts, simplices)
        ob.active_material = self._mat
        be.smooth_shading(ob)
        return

# density surface with color painted to each vertex 
class density_surface(surface_base):
    def __init__(self, name, scene):
        surface_base.__init__(self, name, scene)
        self._mat = be.createVertexColorMaterial('vertex_mat', 'vcol')
        self._subsurf = self._surf.modifiers.new(name = 'subsurf', type = 'SUBSURF')
        return
    # density: n x 3: x, y, density
    # color:  n x 4, RGBA
    # d_scalor: convert to size in plot
    # cutoff: longest allowed edge in simplice
    def swap_surface(self, density, color, d_scalor, cutoff = None, subsurf = None):
        ob = self._surf
        verts = np.append(density[:,:2], np.reshape(density[:,2]*d_scalor, (-1,1)), 1)
        points = density[:,:2]
        tri = Delaunay(points)
        faces = tri.simplices.copy()
        if cutoff is not None:
            faces = [s for s in faces if not self._remove_simplice(cutoff, s, points)]
        be.swap_face(ob, verts, faces)
        vert_color = be.create_vertex_color('vcol', ob, color)
        ob.active_material = self._mat
        be.smooth_shading(ob)
        if subsurf is not None:
            self._subsurf.render_levels = subsurf 
        return

    def _edge_length(self, i, j, verts):
        return sum((verts[i]-verts[j])**2)**.5

    def _remove_simplice(self, cutoff, s, verts):
        l1 = self._edge_length(s[0], s[1], verts)
        l2 = self._edge_length(s[1], s[2], verts)
        l3 = self._edge_length(s[2], s[0], verts)
        return max(l1, l2, l3) > cutoff