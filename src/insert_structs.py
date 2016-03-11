#!/usr/bin/python
import os
import sys

class Field:
    def __init__(self, name, type):
        self.name = name
        self.type = type


class Struct:
    def __init__(self, name):
        self.name = name
        self.fields = []

    def add_field(self, field):
        self.fields.append(field)


def parse_structs(struct_filename):
    struct_file = open(struct_filename, 'r')

    structs = {}

    line = struct_file.readline()
    while len(line) > 0:
        struct_name = line.split()[3]
        line = struct_file.readline()

        line_no = int(line.split()[4])
        line = struct_file.readline()

        struct = Struct(struct_name)

        if line_no not in structs:
            structs[line_no] = []
        structs[line_no].append(struct)

        while not line.startswith('====='):
            tokens = line.split()
            field_name = tokens[len(tokens) - 1]
            type = ' '.join(tokens[0:len(tokens) - 1])

            struct.add_field(Field(field_name, type))

            line = struct_file.readline()

        line = struct_file.readline()

    struct_file.close()

    return structs


if len(sys.argv) != 3:
    print('usage: python insert_structs.py <input-file> <struct-file>')
    sys.exit(1)

input_filename = sys.argv[1]
struct_filename = sys.argv[2]

input_file = open(input_filename, 'r')

structs = parse_structs(struct_filename)

line_no = 1
for line in input_file:
    if line_no in structs:
        for struct in structs[line_no]:
            print('typedef struct _' + struct.name + ' {')
            for field in struct.fields:
                print('    ' + field.type + ' ' + field.name + ';')
            print('} ' + struct.name + ';')

    sys.stdout.write(line)
    line_no += 1

input_file.close()
