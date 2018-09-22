# Copyright(C) 2018 Coos Baakman
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

bl_info = {
    'name': 'Export to xml mesh type (.xml)',
    'author': 'Baakman, Coos',
    'version': '3.2.1',
    'blender': (2, 7, 9),
    'location': 'File > Export',
    'description': 'Export to xml mesh type (.xml)',
    'warning': '',
    'wiki_url': '',
    'tracker_url': '',
    'category': 'Import - Export'
}

import sys
import traceback
from enum import Enum

import bpy
from bpy_extras.io_utils import ExportHelper, path_reference_mode
from math import radians
from mathutils import Matrix, Vector, Quaternion
import xml.etree.ElementTree as ET


class ExportOperator(bpy.types.Operator, ExportHelper):
    """XML exporter"""

    bl_idname = "xml.exporter"
    bl_label = "Export XML Mesh"
    bl_options = {'PRESET'}

    filename_ext = ".xml"
    filter_glob = bpy.props.StringProperty(default="*.xml", options={'HIDDEN'})

    path_mode = path_reference_mode

    def execute(self, context):
        selected_objects = [o for o in context.scene.objects if o.select]
        if len(selected_objects) <= 0:
            raise ValueError("nothing selected")

        keywords = self.as_keywords()
        filepath = keywords['filepath']

        bpy.path.ensure_ext(filepath, '.xml')
        Exporter().export(selected_objects[0], filepath)

        return {'FINISHED'}


class Exporter:
    def __init__(self):
        self.transform_vertex = Matrix.Rotation(radians(-90), 4, 'X')
        self.transform_displacement = get_without_translation(self.transform_vertex)
        self.transform_rotation = get_rotation(self.transform_vertex)

    def export(self, obj, xml_path):
        if obj.type == 'MESH':
            mesh_object = obj
            armature_object = find_armature_for_mesh(obj)
        elif obj.type == 'ARMATURE':
            armature_object = obj
            mesh_object = find_mesh_for_armature(obj)
            if mesh_object is None:
                raise ValueError("no mesh for armature %s" % obj.name)
        else:
            raise TypeError("%s is not a mesh nor armature" % obj.name)

        root = self._to_xml(mesh_object, armature_object)
        indent_xml(root)

        tree = ET.ElementTree(root)
        tree.write(xml_path)

    def _to_xml(self, mesh_object, armature_object):
        mesh_tag = self._mesh_to_xml(mesh_object)
        if armature_object is not None:
            armature_tag = self._armature_to_xml(armature_object, mesh_object)
            mesh_tag.append(armature_tag)
        return mesh_tag

    def _mesh_to_xml(self, mesh_object):
        mesh = mesh_object.data
        mesh_tag = ET.Element('mesh')

        # Convert the vertices:
        vertices_tag = ET.Element('vertices')
        mesh_tag.append(vertices_tag)
        for vertex in mesh.vertices:
            vertex_tag = self._vertex_to_xml(vertex)
            vertices_tag.append(vertex_tag)

        subset_faces = {}

        # Convert the faces:
        faces_tag = ET.Element('faces')
        mesh_tag.append(faces_tag)
        flip_faces = flips_chirality(self.transform_rotation)
        for face in mesh.polygons:
            face_tag = self._face_to_xml(face, mesh.uv_layers, flip_faces)
            faces_tag.append(face_tag)

            if face.material_index not in subset_faces:
                subset_faces[face.material_index] = []
            subset_faces[face.material_index].append(face)

        # If there are subsets, convert them:
        if len(subset_faces) > 0:
            subsets_tag = ET.Element('subsets')
            mesh_tag.append(subsets_tag)
            for material_index in subset_faces:
                if material_index < len(mesh.materials):
                    material = mesh.materials[material_index]
                else:
                    material = None

                subset_tag = self._subset_to_xml(material, subset_faces[material_index], material_index)
                subsets_tag.append(subset_tag)

        return mesh_tag

    def _vertex_to_xml(self, vertex):
        # Register vertex properties as attributes.
        # The entire vertex will be one xml tag.
        vertex_tag = ET.Element('vertex')
        vertex_tag.attrib['id'] = str(vertex.index)
        pos = self.transform_vertex * vertex.undeformed_co
        norm = self.transform_rotation * vertex.normal

        # Vertex position in mesh space:
        pos_tag = ET.Element('pos')
        pos_tag.attrib['x'] = format_float(pos[0])
        pos_tag.attrib['y'] = format_float(pos[1])
        pos_tag.attrib['z'] = format_float(pos[2])
        vertex_tag.append(pos_tag)

        # Vertex normal, used for lighting:
        norm_tag = ET.Element('norm')
        norm_tag.attrib['x'] = format_float(norm[0])
        norm_tag.attrib['y'] = format_float(norm[1])
        norm_tag.attrib['z'] = format_float(norm[2])
        vertex_tag.append(norm_tag)

        return vertex_tag

    def _face_to_xml(self, face, uv_layers, flip_face):
         # Only faces with 3 or 4 faces are supported.
        if len(face.vertices) == 3:
            face_tag = ET.Element('triangle')
        elif len(face.vertices) == 4:
            face_tag = ET.Element('quad')
        else:
            raise ValueError("encountered a face with %i vertices" % len(face.vertices))

        face_tag.attrib['id'] = str(face.index)
        face_tag.attrib['smooth'] = str(face.use_smooth).lower()

        # Order determines which side is front.
        vertex_order = range(len(face.vertices))
        if flip_face:
            vertex_order.reverse()
        for i in vertex_order:
            corner_tag = ET.Element('corner')
            face_tag.append(corner_tag)

            # Reference to vertex:
            corner_tag.attrib['vertex_id'] = str(face.vertices[i])

            # Texture coordinates:
            if uv_layers.active is not None:
                corner_tag.attrib['tex_u'] = format_float(uv_layers.active.data[face.loop_indices[i]].uv[0])
                corner_tag.attrib['tex_v'] = format_float(uv_layers.active.data[face.loop_indices[i]].uv[1])
            else:
                corner_tag.attrib['tex_u'] = 0.0
                corner_tag.attrib['tex_v'] = 0.0

        return face_tag

    def _subset_to_xml(self, material, faces, subset_index):
        # Take the material color settings.
        # We might need to add more in the future..
        if material is not None:
            id_ = material.name
        else:
            # No material associated with subset, use default values:
            id_ = str(subset_index)

        subset_tag = ET.Element('subset')

        # Add id to tag, makes it easier to find back in the file.
        subset_tag.attrib['id'] = id_

        # Register all the subset's faces in xml tags:
        faces_tag = ET.Element('faces')
        subset_tag.append(faces_tag)
        for face in faces:
            if len(face.vertices) == 4:
                face_tag = ET.Element('quad')
            elif len(face.vertices) == 3:
                face_tag = ET.Element('triangle')
            else:
                raise ValueError("encountered a face with %i vertices" % len(face.vertices))

            faces_tag.append(face_tag)
            face_tag.attrib['id'] = str(face.index)

        return subset_tag


    def _armature_to_xml(self, armature_object, mesh_object):
        armature_tag = ET.Element('armature')

        armature = armature_object.data
        mesh = mesh_object.data

        # need to map vertices by group:
        vertices_by_group = {}
        for vertex in mesh.vertices:
            for object_group in mesh_object.vertex_groups:
                for vg in vertex.groups:
                    if object_group.index == vg.group:
                        if object_group.name not in vertices_by_group:
                            vertices_by_group[object_group.name] = []
                        vertices_by_group[object_group.name].append(vertex.index)

        armature_to_world = armature_object.matrix_world
        world_to_mesh = mesh_object.matrix_world.inverted()
        armature_to_mesh = world_to_mesh * armature_to_world

        bones_tag = ET.Element('bones')
        armature_tag.append(bones_tag)

        # Iterate over the armature's bones:
        for bone in armature.bones:
            # A tag for every bone:
            bone_tag = ET.Element('bone')
            bones_tag.append(bone_tag)
            bone_tag.attrib['id'] = bone.name

            # Export position of bone head and tail in mesh space:
            headpos = self.transform_vertex * armature_to_mesh * bone.head_local
            tailpos = self.transform_vertex * armature_to_mesh * bone.tail_local
            bone_tag.attrib['x'] = format_float(headpos[0])
            bone_tag.attrib['y'] = format_float(headpos[1])
            bone_tag.attrib['z'] = format_float(headpos[2])
            bone_tag.attrib['tail_x'] = format_float(tailpos[0])
            bone_tag.attrib['tail_y'] = format_float(tailpos[1])
            bone_tag.attrib['tail_z'] = format_float(tailpos[2])
            bone_tag.attrib['weight'] = format_float(1.0)

            if bone.parent is not None:
                 bone_tag.attrib['parent_id'] = bone.parent.name

            if bone.name in vertices_by_group:
                # Register references to the verices that this bone pulls at:
                vertices_tag = ET.Element('vertices')
                bone_tag.append(vertices_tag)

                for vertex_index in vertices_by_group[bone.name]:
                    vertex_tag = ET.Element('vertex')
                    vertices_tag.append(vertex_tag)
                    vertex_tag.attrib['id'] = str(vertex_index)

        # The armature might have animations associated with it.
        animations_tag = self._armature_animations_to_xml(armature_object, armature_to_mesh)
        armature_tag.append(animations_tag)

        return armature_tag

    def _armature_animations_to_xml(self, armature_object, armature_to_mesh):
        animations_tag = ET.Element('animations')

        bones = armature_object.data.bones

        rot_matrix = self.transform_rotation * get_rotation(armature_to_mesh)
        loc_matrix = self.transform_displacement * armature_to_mesh

        # Check for chirality flip in the rotation transformation:
        chirality_flip = flips_chirality(self.transform_rotation)

        # Check every action to see if it's an animation:
        for action in bpy.data.actions:
            action_start, action_end = action.frame_range
            layer_tags = []
            for group in action.groups:
                if group.name not in bones:
                    continue
                bone = bones[group.name]
                bone_matrix = bone.matrix_local

                layer_tag = ET.Element('layer')
                layer_tags.append(layer_tag)
                layer_tag.attrib['bone_id'] = group.name

                # Collect the data from the curves:
                ordering = {}
                type_ = None
                for fcurve in group.channels:
                    type_ = get_curve_property_type(fcurve.data_path, fcurve.array_index)

                    for keyframe in fcurve.keyframe_points:
                        time, value = keyframe.co
                        time = int(time)
                        if time not in ordering:
                            ordering[time] = {}
                        ordering[time][type_] = value

                # Store the ordered values in tags:
                for time in ordering:
                    key_tag = ET.Element('key')
                    key_tag.attrib['frame'] = str(int(time - action_start))
                    layer_tag.append(key_tag)

                    properties = ordering[time]
                    rot = extract_rot(properties)
                    if rot is None:
                        rot = Vector((0.0, 0.0, 0.0, 1.0))
                    rot = rot_matrix * get_rotation(bone_matrix) * rot
                    rot = Quaternion((rot.w, rot.x, rot.y, rot.z))
                    if chirality_flip:
                        rot.invert()
                    key_tag.attrib['rot_x'] = format_float(rot.x)
                    key_tag.attrib['rot_y'] = format_float(rot.y)
                    key_tag.attrib['rot_z'] = format_float(rot.z)
                    key_tag.attrib['rot_w'] = format_float(rot.w)

                    loc = extract_loc(properties)
                    if loc is None:
                        loc = Vector((0.0, 0.0, 0.0))
                    loc = loc_matrix * get_without_translation(bone_matrix) * loc
                    key_tag.attrib['x'] = format_float(loc.x)
                    key_tag.attrib['y'] = format_float(loc.y)
                    key_tag.attrib['z'] = format_float(loc.z)

            if len(layer_tags) > 0:
                animation_tag = ET.Element('animation')
                animation_tag.attrib['id'] = action.name
                animations_tag.append(animation_tag)
                animation_tag.extend(layer_tags)

                length = int(action_end - action_start)
                animation_tag.attrib['length'] = str(length)

        return animations_tag


def get_without_translation(matrix):
    return matrix.copy().to_3x3().to_4x4()


def get_rotation(matrix):
    return get_without_translation(matrix).to_quaternion().to_matrix().to_4x4()


class CurvePropertyType(Enum):
    ROT_W = 'rot_w',
    ROT_X = 'rot_x',
    ROT_Y = 'rot_y',
    ROT_Z = 'rot_z',
    LOC_X = 'x',
    LOC_Y = 'y',
    LOC_Z = 'z'


def get_curve_property_type(path, index):
    """maps fcurve data to variable ids"""

    if path.endswith('.rotation_quaternion'):
        if index == 0:
            return CurvePropertyType.ROT_W
        elif index == 1:
            return CurvePropertyType.ROT_X
        elif index == 2:
            return CurvePropertyType.ROT_Y
        elif index == 3:
            return CurvePropertyType.ROT_Z
    elif path.endswith('.location'):
        if index == 0:
            return CurvePropertyType.LOC_X
        elif index == 1:
            return CurvePropertyType.LOC_Y
        elif index == 2:
            return CurvePropertyType.LOC_Z
    elif path.endswith('.scale'):
        raise TypeError("scale bone modifiers are not supported")
    else:
        raise TypeError("unsupported bone modifier: \"%s\"" % path)


def extract_rot(properties):
    """returns None if data is missing"""

    if CurvePropertyType.ROT_W in properties and \
       CurvePropertyType.ROT_X in properties and \
       CurvePropertyType.ROT_Y in properties and \
       CurvePropertyType.ROT_Z in properties:
        return Vector((properties[CurvePropertyType.ROT_X],
                       properties[CurvePropertyType.ROT_Y],
                       properties[CurvePropertyType.ROT_Z],
                       properties[CurvePropertyType.ROT_W]))
    else:
        return None


def extract_loc(properties):
    """returns None if data is missing"""

    if CurvePropertyType.LOC_X in properties and \
       CurvePropertyType.LOC_Y in properties and \
       CurvePropertyType.LOC_Z in properties:
        return Vector((properties[CurvePropertyType.LOC_X],
                       properties[CurvePropertyType.LOC_Y],
                       properties[CurvePropertyType.LOC_Z]))
    else:
        return None


def color_as_list(color):
    """basically converts a color object to a list of length 3"""

    return [color.r, color.g, color.b]


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

def flips_chirality(matrix):
    """whether or not the matrix turns meshes into their mirror image"""

    return matrix.determinant() < 0.0


def format_float(f):
    """The floating point as written to the file"""

    return '{:.4e}'.format(f)


def find_armature_for_mesh(mesh_object):
    for modifier in mesh_object.modifiers:
        if modifier.type == 'ARMATURE':
            return modifier.object
    return None


def find_mesh_for_armature(armature_object):
    for candidate_object in bpy.context.scene.objects:
        if candidate_object.type == 'MESH':
            for modifier in candidate_object.modifiers:
                if modifier.type == 'ARMATURE':
                    if modifier.object.name == armature_object.name:
                        return candidate_object
    return None


def find_object(object_name):
    if object_name in bpy.context.scene.objects:
        return bpy.context.scene.objects[object_name]
    return None


def menu_func_export(self, context):
    self.layout.operator(ExportOperator.bl_idname,
                         text="XML Mesh (.xml)")


def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)


if __name__ == '__main__':
    script_args = []
    if '--' in sys.argv:
        script_args = sys.argv[sys.argv.index('--') + 1:]

    if len(script_args) >= 2:
        try:
            object_name = script_args[0]
            xml_path = script_args[1]

            obj = find_object(object_name)
            if obj is None:
                raise ValueError("object %s not found" % object_name)

            Exporter().export(obj, xml_path)
        except:
            traceback.print_exc()
            sys.exit(1)
