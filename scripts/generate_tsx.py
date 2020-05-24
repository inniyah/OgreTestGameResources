#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import json
import sys
import os

from data_db import get_obj_info, update_obj_info, get_all_info
from obj_loader import ObjLoader

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Process obj 3d models")
    parser.add_argument("-d", "--db", help="path to the json db", default=None)
    parser.add_argument("-i", "--input", help="input directory", default=None)
    parser.add_argument("-o", "--output", help="output directory", default=None)
    parser.add_argument("-n", "--name", help="tiles name", default=None)
    args = parser.parse_args()

    all_info = get_all_info(args.db)

    if not args.db or not args.input or not args.output or not args.name:
        print("Wrong arguments")
        sys.exit(-1)

    for (tiles_name, tiles_list, tile_id_base, tsx_filename) in [
        (args.name, all_info[":type_lists"][args.name], 1, os.path.join(args.output, f"{args.name}.tsx")),
    ]:

        tiles_per_object = 4
        tile_id_multiplier = 8

        num_of_objects = len(tiles_list)

        with open(tsx_filename, 'w') as file_output:

            print(f'<?xml version="1.0" encoding="UTF-8"?>', file=file_output)
            print(f'<tileset version="1.2" tiledversion="1.3.3" name="{tiles_name}" tilewidth="128" tileheight="224" tilecount="{num_of_objects}" columns="0">', file=file_output)
            print(f' <grid orientation="orthogonal" width="1" height="1"/>', file=file_output)

            for tile_id in sorted(tiles_list.keys()):
                obj_filename = tiles_list[tile_id]
                tile_id_num = int(tile_id)
                tile_info = all_info[obj_filename]

                if ":attrs_file" in tile_info.keys():
                    try:
                        with open(tile_info[":attrs_file"]) as json_file:
                            json_data = json.load(json_file)

                            try:
                                attrs_data = json_data[""]
                            except KeyError:
                                attrs_data = { }

                            try:
                                new_attrs_data = json_data[os.path.splitext(obj_filename)[0]]
                            except KeyError:
                                new_attrs_data = { }

                            del json_data
                            attrs_data.update(new_attrs_data)

                    except FileNotFoundError:
                        attrs_data = { }

                for (tile_id_inc, json_label, rot_angle) in [
                    (0, "@IsoTile1", 0),
                    (1, "@IsoTile2", 90),
                    (2, "@IsoTile3", 180),
                    (3, "@IsoTile4", 270),
                ]:

                    print(f' <tile id="{tile_id_base + tile_id_multiplier * (tile_id_num - 1) + tile_id_inc}">', file=file_output)
                    print(f'  <properties>', file=file_output)
                    print(f'   <property name="3DModel" value="{tile_info["@3DModel"]}"/>', file=file_output)
                    print(f'   <property name="3DMesh" value="{os.path.basename(tile_info["@3DMesh"])}"/>', file=file_output)
                    print(f'   <property name="Material" value="{os.path.basename(tile_info["@Material"])}"/>', file=file_output)
                    print(f'   <property name="RotAngle" value="{rot_angle}"/>', file=file_output)

                    try:
                        print(f'   <property name="Texture" value="{tile_info["@Texture"]}"/>', file=file_output)
                    except KeyError:
                        pass

                    for pname, pvalue in attrs_data.items():
                        print(f'   <property name="{pname}" value="{pvalue}"/>', file=file_output)

                    print(f'  </properties>', file=file_output)
                    print(f'  <image source="{os.path.relpath(args.input, args.output)}/{tile_info[json_label]}" width="128" height="224"/>', file=file_output)
                    print(f' </tile>', file=file_output)

            print(f'</tileset>', file=file_output)
