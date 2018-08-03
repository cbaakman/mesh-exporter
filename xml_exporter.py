#!BPY

# Copyright(C) 2015 Coos Baakman
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.


"""
# Name: 'xml mesh type(.xml)...'
# Blender: 249
# Group: 'Export'
# Tooltip: 'Export to xml file format.'
"""
__author__ = "Coos Baakman"
__version__ = "2.1"
__bpydoc__ = """\
This script exports a Blender mesh to an xml formatted file
format.
"""

import Blender
from Blender import Types, Object, Material, Armature, Mesh, Modifier
from Blender.Mathutils import *
from Blender import Draw, BGL
from Blender.BGL import *
from Blender.IpoCurve import ExtendTypes, InterpTypes
import math
import xml.etree.ElementTree as ET


EXTENSION='.xml'


def callback_selector(filename):
    # A file path has been selected to export to
    if not filename.endswith(EXTENSION):
        filename += EXTENSION
    exporter = Exporter()
    exporter.export_selected(filename)


def draw():
    # Make the menu in Blender: a single button to press.
    glClearColor(0.55, 0.6, 0.6, 1.0)
    glClear(GL_COLOR_BUFFER_BIT)
    glRasterPos2d(20, 75)
    Draw.Button("Export Selected", 1, 20, 155, 100, 30, "export the selected object")


def event(evt, val):
    if evt == Draw.ESCKEY:
        Draw.Exit()


def button_event(evt):
    if evt == 1:
        fname = Blender.sys.makename(ext = EXTENSION)
        Blender.Window.FileSelector(callback_selector, "Export xml mesh", fname)
        Draw.Exit()


# Register the exported menu:
Draw.Register(draw, event, button_event)


def find_mesh_armature(mesh_object):
    """Find an armature for a given mesh object."""

    for mod in mesh_object.modifiers:
        if mod.type == Modifier.Types.ARMATURE:
            # Return as an object:
            return mod[Modifier.Settings.OBJECT]

    return None


def find_armature_mesh(armature_object):
    """Find a mesh, to which the armature belongs."""

    armature_name = armature_object.getData(True, False)
    for candidate_object in Object.Get():
        # Skip objects that are not meshes
        if type(candidate_object.getData(False, True)) != Types.MeshType:
            continue

        for mod in candidate_object.modifiers:
            # Skip modifiers that are not armatures
            if mod.type != Modifier.Types.ARMATURE:
                continue

            candidate_name = mod[Modifier.Settings.OBJECT].getData(True, False)
            if candidate_name == armature_name:
                # This is my armature !
                return candidate_object
    return None


def flips_chirality(matrix):
    """returns true if this matrix generates a mirror image of the mesh"""

    return matrix.determinant() < 0


def indent_xml(elem, level=0):
    """makes sure the xml tree becomes a formatted text file,
       not just a single line
    """

    s = "\n" + level * "  "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = s + "  "
        if not elem.tail or not elem.tail.strip():
            elem.tail = s
        for elem in elem:
            indent_xml(elem, level + 1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = s
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = s


def format_float(f):
    """the float as is gets saved to the xml file"""

    return '{0:.4e}'.format(f)


class Exporter(object):
    # Code used in this exporter is based on the following documentation:
    # http://www.blender.org/api/249PythonDoc/

    def __init__(self):
        # Switch y and z, without mirroring:
        self.transform_vertex = RotationMatrix(90, 4, 'x')

        # Used on bone translocations:
        self.transform_displacement = Matrix(self.transform_vertex)
        for i in range(3):
            self.transform_displacement[3][i] = 0
            self.transform_displacement[i][3] = 0
        self.transform_displacement[3][3] = 1

        # Used on normals and bone rotations:
        self.transform_rotation = Matrix(self.transform_displacement)
        self.transform_rotation.invert()
        self.transform_rotation.transpose()

    def export_selected(self, filename):
        """checks for a selected mesh or armature,
           prompts the user for a filename and
           saves the mesh in xml format
        """

        # Find the selected mesh(and maybe its armature)
        mesh_object = None
        armature_object = None

        # The selected object might actually be an armature, with a mesh attached to it.
        selected = Object.GetSelected()

        if len(selected) == 0:
            raise ValueError("No selection")

        selected_data = selected[0].getData(False, True)

        if type(selected_data) == Types.MeshType:

            mesh_object = selected[0]
            armature_object = find_mesh_armature(mesh_object)

        elif type(selected_data) == Types.ArmatureType:

            armature_object = selected[0]
            mesh_object = find_armature_mesh(armature_object)
            if mesh_object is None:
                raise ValueError("No mesh attached to selected armature")
        else:
            raise TypeError("The selected object %s is not a mesh or armature." % str(type(selected[0])))

        root = self._to_xml(mesh_object, armature_object)
        indent_xml(root)

        tree = ET.ElementTree(root)
        tree.write(filename)

    def _to_xml(self, mesh_object, armature_object=None):
        mesh = self._mesh_to_xml(mesh_object)
        if armature_object is not None:
            tag_armature = self._armature_to_xml(armature_object, mesh_object)
            mesh.append(tag_armature)

        return mesh

    def _mesh_to_xml(self, mesh_object):
        flip_faces = flips_chirality(self.transform_rotation)

        mesh = mesh_object.getData(False, True)
        root = ET.Element('mesh')

        # Convert the mesh's vertices to xml tags:
        tag_vertices = ET.Element('vertices')
        root.append(tag_vertices)

        for vertex in mesh.verts:
            tag_vertex = self._vertex_to_xml(vertex)
            tag_vertices.append(tag_vertex)

        # Convert the mesh's faces to xml tags:
        tag_faces = ET.Element('faces')
        root.append(tag_faces)

        # Divide the mesh in subsets, based on their materials.
        # This allows us to render different parts of the mesh with different OpenGL settings,
        # like material-specific ones.
        subset_faces = {}
        for face in mesh.faces:
            tag_face = self._face_to_xml(face, flip_faces)
            tag_faces.append(tag_face)

            if face.mat not in subset_faces:
                subset_faces[face.mat] = []

            subset_faces[face.mat].append(face)

        # Did we even find any subsets?
        if len(subset_faces) > 0:
            tag_subsets = ET.Element('subsets')
            root.append(tag_subsets)
            for material_i in subset_faces:
                if material_i < len(mesh.materials):
                    material = mesh.materials[material_i]
                else:
                    # material index is not found in mesh
                    material = None

                tag_subset = self._subset_to_xml(material, subset_faces[material_i])
                tag_subset.attrib['id'] = str(material_i)
                tag_subsets.append(tag_subset)

        return root

    def _subset_to_xml(self, material, faces):
        # Take the material color settings.
        # We might need to add more in the future..
        if material is not None:
            name = material.name
            diffuse  = material.rgbCol + [1.0]
            specular = material.specCol + [1.0]
            emission = [material.emit * diffuse[i] for i in range(4)]
        else:
            # No material associated with subset, use default values:
            name = 'none'
            diffuse  = [1.0, 1.0, 1.0, 1.0]
            specular = [0.0, 0.0, 0.0, 0.0]
            emission = [0.0, 0.0, 0.0, 0.0]

        tag_subset = ET.Element('subset')

        # Add name to tag, makes it easier to find back in the file.
        if name:
            tag_subset.attrib['name'] = str(name)

        # Convert material colors to xml attributes
        c = ['r', 'g', 'b', 'a']
        tag_diffuse = ET.Element('diffuse')
        tag_subset.append(tag_diffuse)
        tag_specular = ET.Element('specular')
        tag_subset.append(tag_specular)
        tag_emission = ET.Element('emission')
        tag_subset.append(tag_emission)
        for i in range(4):
            tag_diffuse.attrib[c[i]] = format_float(diffuse[i])
            tag_specular.attrib[c[i]] = format_float(specular[i])
            tag_emission.attrib[c[i]] = format_float(emission[i])

        # Register all the subset's faces in xml tags:
        tag_faces = ET.Element('faces')
        tag_subset.append(tag_faces)
        for face in faces:
            if len(face.verts) == 4:
                tag_face = ET.Element('quad')
            elif len(face.verts) == 3:
                tag_face = ET.Element('triangle')
            else:
                raise ValueError("encountered a face with %i vertices" % len(face.verts))

            tag_faces.append(tag_face)
            tag_face.attrib['id'] = str(face.index)

        return tag_subset

    def _face_to_xml(self, face, flip_face):
        # Only faces with 3 or 4 faces are supported.
        if len(face.verts) == 3:
            tag_face = ET.Element('triangle')
        elif len(face.verts) == 4:
            tag_face = ET.Element('quad')
        else:
            raise ValueError("encountered a face with %i vertices" % len(face.v))

        tag_face.attrib['id'] = str(face.index)
        tag_face.attrib['smooth'] = str(face.smooth)

        # Order determines which side is front.
        vertex_order = range(len(face.verts))
        if flip_face:
            vertex_order.reverse()
        for i in vertex_order:
            tag_corner = ET.Element('corner')
            tag_face.append(tag_corner)

            # Reference to vertex:
            tag_corner.attrib['vertex_id'] = str(face.verts[i].index)

            # Texture coordinates:
            try:
                tag_corner.attrib['tex_u'] = format_float(face.uv[i][0])
                tag_corner.attrib['tex_v'] = format_float(face.uv[i][1])
            except:
                tag_corner.attrib['tex_u'] = '0.0'
                tag_corner.attrib['tex_v'] = '0.0'

        return tag_face

    def _vertex_to_xml(self, vertex):
        # Register vertex properties as attributes.
        # The entire vertex will be one xml tag.
        tag_vertex = ET.Element('vertex')
        tag_vertex.attrib['id'] = str(vertex.index)
        co = self.transform_vertex * vertex.co
        no = self.transform_rotation * vertex.no

        # Vertex position in space:
        tag_pos = ET.Element('pos')
        tag_pos.attrib['x'] = format_float(co[0])
        tag_pos.attrib['y'] = format_float(co[1])
        tag_pos.attrib['z'] = format_float(co[2])
        tag_vertex.append(tag_pos)

        # Vertex normal, used for lighting:
        tag_pos = ET.Element('norm')
        tag_pos.attrib['x'] = format_float(no[0])
        tag_pos.attrib['y'] = format_float(no[1])
        tag_pos.attrib['z'] = format_float(no[2])
        tag_vertex.append(tag_pos)

        return tag_vertex

    def _armature_to_xml(self, armature_object, mesh_object):
        armature = armature_object.getData()
        mesh = mesh_object.getData(False, True)
        vertex_groups = mesh.getVertGroupNames()

        armature_to_world = armature_object.getMatrix('worldspace').copy().invert()
        world_to_mesh = mesh_object.getMatrix('worldspace')

        armature_to_mesh = world_to_mesh * armature_to_world

        root = ET.Element('armature')
        bones_tag = ET.Element('bones')
        root.append(bones_tag)

        # Iterate over the armature's bones:
        bone_list = []
        for bone_name in armature.bones.keys():
            bone_tag = ET.Element('bone')
            bones_tag.append(bone_tag)

            # A tag for every bone:
            bone = armature.bones[bone_name]
            bone_tag.attrib['id'] = bone_name
            bone_list.append(bone)

            # Export position of bone head and tail in mesh space:
            headpos = self.transform_vertex * armature_to_mesh * bone.head['ARMATURESPACE']
            tailpos = self.transform_vertex * armature_to_mesh * bone.tail['ARMATURESPACE']
            bone_tag.attrib['x'] = format_float(headpos[0])
            bone_tag.attrib['y'] = format_float(headpos[1])
            bone_tag.attrib['z'] = format_float(headpos[2])
            bone_tag.attrib['tail_x'] = format_float(tailpos[0])
            bone_tag.attrib['tail_y'] = format_float(tailpos[1])
            bone_tag.attrib['tail_z'] = format_float(tailpos[2])
            bone_tag.attrib['weight'] = format_float(bone.weight)

            # Reference to parent bone:
            if bone.hasParent():
                bone_tag.attrib['parent_id'] = bone.parent.name

            if bone_name in mesh.getVertGroupNames():
                # Register references to the verices that this bone pulls at:
                vertices_tag = ET.Element('vertices')
                bone_tag.append(vertices_tag)

                for vertex_index in mesh.getVertsFromGroup(bone_name, False):
                    vertex_tag = ET.Element('vertex')
                    vertices_tag.append(vertex_tag)
                    vertex_tag.attrib['id'] = str(vertex_index)

        # The armature might have animations associated with it.
        animations_tag = self._armature_animations_to_xml(armature_object, armature_to_mesh, bone_list)
        root.append(animations_tag)

        return root

    def _armature_animations_to_xml(self, armature_object, armature_to_mesh, bone_list):
        root = ET.Element('animations')

        rot_matrix = self.transform_rotation * armature_to_mesh.rotationPart().resize4x4()
        loc_matrix = self.transform_displacement * armature_to_mesh

        # Check for chirality flip in the rotation transformation:
        chirality_flip = flips_chirality(self.transform_rotation)

        # Check every action to see if it's an animation:
        for action in Armature.NLA.GetActions().values():

            animation_tag = ET.Element('animation')

            count_layers = 0

            # Check every known bone:
            for bone in bone_list:
                action.setActive(armature_object)

                if bone.name not in action.getChannelNames():
                    continue

                ipo = action.getChannelIpo(bone.name)

                if len(ipo.curves) <= 0:
                    # Bone doesn't move in this action
                    continue

                # A layer for every moving bone:
                layer_tag = ET.Element('layer')
                layer_tag.attrib['bone_id'] = bone.name
                animation_tag.append(layer_tag)

                count_layers += 1

                # Find all key frames for this bone:
                frame_numbers = []
                for curve in ipo.curves:
                    for bez_triple in curve.bezierPoints:
                        frame_num = int(bez_triple.pt[0])
                        if frame_num not in frame_numbers:
                            frame_numbers.append(frame_num)

                # Iterate over key frames
                for frame_num in frame_numbers:
                    # Get pose at specified frame number:
                    Blender.Set("curframe", frame_num)
                    pose_bone = armature_object.getPose().bones[bone.name]

                    # Register frame number, position and rotation of the bone:
                    rot = pose_bone.localMatrix.toQuat()
                    rot = rot_matrix * Vector(rot.x, rot.y, rot.z, rot.w)
                    rot = Quaternion(rot.w, rot.x, rot.y, rot.z)
                    if chirality_flip:
                        rot = rot.inverse()

                    loc = pose_bone.loc
                    loc = loc_matrix * loc

                    key_tag = ET.Element('key')
                    layer_tag.append(key_tag)

                    key_tag.attrib['frame'] = str(frame_num)
                    key_tag.attrib['x'] = format_float(loc.x)
                    key_tag.attrib['y'] = format_float(loc.y)
                    key_tag.attrib['z'] = format_float(loc.z)
                    key_tag.attrib['rot_x'] = format_float(rot.x)
                    key_tag.attrib['rot_y'] = format_float(rot.y)
                    key_tag.attrib['rot_z'] = format_float(rot.z)
                    key_tag.attrib['rot_w'] = format_float(rot.w)

            if count_layers > 0:  # means the action applies to this armature
                animation_tag.attrib['name'] = action.name
                animation_length = max(action.getFrameNumbers())

                animation_tag.attrib['length'] = str(animation_length)
                root.append(animation_tag)

        return root
