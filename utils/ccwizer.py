# this was a waste of time!

import numpy as np
import argparse
import re

def sub(u, v):
    w = {}
    w['x'] = u['x'] - v['x']
    w['y'] = u['y'] - v['y']
    w['z'] = u['z'] - v['z']
    return w

def cross(u, v):
    w = {}
    w['x'] =  (u['y'] * v['z']) - (u['z'] * v['y'])
    w['y'] = -(u['x'] * v['z']) + (u['z'] * v['x'])
    w['z'] =  (u['x'] * v['y']) - (u['y'] * v['x'])
    return w

def dot(u, v):
    d = (u['x'] * v['x']) + (u['y'] * v['y']) + (u['z'] * v['z'])
    return d

def main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument("filename")
    args = arg_parser.parse_args()

    file = open(args.filename, "r")
    file_lines = file.readlines()

    triangles = []
    vertex_count = 0

    for line in file_lines:
        found_v4 = re.findall(r"[-+]?(?:\d*\.*\d+)", line)[1:]

        if len(found_v4) == 4:
            found_v3 = [float(n) for n in found_v4[:3]]

            v = {}

            v['x'] = found_v3[0]
            v['y'] = found_v3[1]
            v['z'] = found_v3[2]

            if vertex_count % 3 == 0:
                triangles.append([])

            triangles[-1].append(v)
            vertex_count += 1

    z = {}
    z['x'] = 0
    z['y'] = 0
    z['z'] = 1

    for triangle in triangles:
        v10 = sub(triangle[1], triangle[0])
        v20 = sub(triangle[2], triangle[0])

        crossed = cross(v10, v20)
        print(dot(crossed, z))
        


if __name__ == "__main__":
    main()

