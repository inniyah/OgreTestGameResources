# See: https://github.com/totex/PyOpenGL_tutorials/blob/master/ObjLoader.py

import sys
import os

class ObjLoader:
    def __init__(self):
        self.vert_coords = []
        self.text_coords = []
        self.norm_coords = []
        self.vert_colors = []

        self.vertex_index = []
        self.texture_index = []
        self.normal_index = []

        self.materials = {}

        self.object = None
        self.mtllib = None
        self.usemtl = None
        self.smooth = None

    def load_material(self, file):
        matname = os.path.splitext(os.path.basename(file))[0]
        matdata = None

        class Material:
            def __init__(self, path):
                self.path = path
                self.Ns = None
                self.Ni = None
                self.Ka = None
                self.Kd = None
                self.Ks = None
                self.Ke = None
                self.d = None
                self.illum = None
                self.map_Kd = None
            def __repr__(self):
                return f"Material['{self.map_Kd}']"
            def __str__(self):
                return f"Material['{self.map_Kd}']"
            def getKdImage(self):
                return Image.open(os.path.join(os.path.dirname(self.path), self.map_Kd))

        for line in open(file, 'r'):
            if line.lstrip().startswith('#'): continue
            values = line.split()
            if not values: continue

            if values[0] == 'newmtl':
                matname = values[1]
                matdata = Material(file)
            if values[0] == 'Ns':
                matdata.Ns = values[1]
            if values[0] == 'Ni':
                matdata.Ni = values[1]
            if values[0] == 'Ka':
                matdata.Ka = values[1:4]
            if values[0] == 'Kd':
                matdata.Kd = values[1:4]
            if values[0] == 'Ks':
                matdata.Ks = values[1:4]
            if values[0] == 'Ke':
                matdata.Ke = values[1:4]
            if values[0] == 'd':
                matdata.d = values[1]
            if values[0] == 'illum':
                matdata.illum = values[1]
            if values[0] == 'map_Kd':
                matdata.map_Kd = values[1]

        self.materials[matname] = matdata

    def load_model(self, file):
        normals = False
        texture = False
        vcolors = False
        for line in open(file, 'r'):
            if line.lstrip().startswith('#'): continue
            values = line.split()
            if not values: continue

            if values[0] == 'o':
                self.object = values[1]

            if values[0] == 's':
                self.smooth = values[1]

            if values[0] == 'mtllib':
                self.mtllib = values[1]

                mtllib = os.path.join(os.path.dirname(file), values[1])
                self.load_material(mtllib)

            if values[0] == 'usemtl':
                self.usemtl = values[1]

                self.material = self.materials[values[1]]

            if values[0] == 'v':
                self.vert_coords.append([float(v) for v in values[1:4]])
                if len(values) >= 7:
                    vcolors = True
                    self.vert_colors.append(values[4:7])
            if values[0] == 'vt':
                texture = True
                self.text_coords.append([float(v) for v in values[1:3]])
            if values[0] == 'vn':
                normals = True
                self.norm_coords.append([float(v) for v in values[1:4]])

            if values[0] == 'f':
                face_i = []
                text_i = []
                norm_i = []
                for v in values[1:]:
                    w = v.split('/')
                    face_i.append(int(w[0])-1)
                    if texture:
                        text_i.append(int(w[1])-1)
                    if normals:
                        norm_i.append(int(w[2])-1)
                self.vertex_index.append([face_i])
                if texture:
                    self.texture_index.append([text_i])
                if normals:
                    self.normal_index.append([norm_i])

        self.vertex_index = [y for x in self.vertex_index for y in x]
        self.texture_index = [y for x in self.texture_index for y in x]
        self.normal_index = [y for x in self.normal_index for y in x]

        if texture:
            assert(len(self.vertex_index) == len(self.texture_index))
        if normals:
            assert(len(self.vertex_index) == len(self.normal_index))

    def calc_boundaries(self):
        min_x = float('inf')
        min_y = float('inf')
        min_z = float('inf')

        max_x = float('-inf')
        max_y = float('-inf')
        max_z = float('-inf')

        for x, y, z in self.vert_coords:
            min_x = min(min_x, float(x))
            min_y = min(min_y, float(y))
            min_z = min(min_z, float(z))

            max_x = max(max_x, float(x))
            max_y = max(max_y, float(y))
            max_z = max(max_z, float(z))

        return (min_x, min_y, min_z), (max_x, max_y, max_z)

    def print_model(self, file=sys.stderr):
        def w(*args, **kwargs):
            print(*args, file=file, **kwargs)

        if not self.mtllib is None:
            w(f"mtllib {self.mtllib}")
        if not self.object is None:
            w(f"o {self.object}")
        for v in self.vert_coords:
            w("v " + ' '.join([f"{n:.6f}" for n in v]))
        for vt in self.text_coords:
            w("vt " + ' '.join([f"{n:.4f}" for n in vt]))
        for vn in self.norm_coords:
            w("vn " + ' '.join([f"{n:.4f}" for n in vn]))
        if not self.usemtl is None:
            w(f"usemtl {self.usemtl}")
        if not self.smooth is None:
            w(f"s {self.smooth}")
        for vl, tl, nl in zip(self.vertex_index, self.texture_index, self.normal_index):
            w("f " + ' '.join([f"{vi+1}/{ti+1}/{ni+1}" for vi, ti, ni in zip(vl, tl, nl)]))
