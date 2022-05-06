###############################################################################
#               :
#   File        :   code_gen.py
#               :
#   Author(s)   :   Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
#               :
#   Description :   Makefile code generation
#               :   -> fixes errors in STM32CubeMX generated Makefile
#               :   -> adds SUFST code to includes / sources
#               :
###############################################################################

import os

###############################################################################
# print utilities
###############################################################################

class Colours:
    """Colours for printing
    """
    Magenta = '\033[95m'
    Blue = '\033[94m'
    Cyan = '\033[96m'
    Green = '\033[92m'
    Yellow = '\033[93m'
    Red = '\033[91m'
    End = '\033[0m'
    Bold = '\033[1m'
    Underline = '\033[4m'

def print_header(header: str):
    print(Colours.Magenta + Colours.Bold + header + Colours.End)

def print_success(message: str):
    print(Colours.Green + message + Colours.End)

def print_deleted(line: str):
    print(Colours.Red + 'D: ' + Colours.End + line, end='')

def print_modified(original: str, new: str):
    print(Colours.Yellow + 'M: ' + Colours.End + original + ' -> ' + new)

def print_added(line: str):
    print(Colours.Green + 'A: ' + Colours.End + line, end='')

###############################################################################
# SUFST sources and includes
###############################################################################

def add_sufst_files(makefile):

    print_header('\nAdding SUFST files')

    inc_root = 'SUFST/Inc'
    src_root = 'SUFST/Src'
    src_files = []
    inc_dirs = []
    output = []
    changes = 0

    # generate list of .c sources not in makefile
    potential_src_files = []

    for path, subdirs, files in os.walk(src_root):
        
        for f in files:
            
            if f.split('.')[-1] == 'c':
                
                source = os.path.join(path, f)
                potential_src_files.append(source)

    for source in potential_src_files:
        
        add_source = True

        for line in makefile:
            if source in line:
                add_source = False
                break

        if add_source:
            src_files.append(source)
                    
    # generate list of include directories not in makefile
    potential_inc_dirs = []
    potential_inc_dirs.append(inc_root)
    
    for path, subdirs, files in os.walk(inc_root):
        for dir in subdirs:
            include = os.path.join(path, dir)
            potential_inc_dirs.append(include)
            
    for include in potential_inc_dirs:      
        
        add_include = True 

        for line in makefile: 
            if include in line: 
                add_include = False 
                break 

        if add_include:
            inc_dirs.append(include)

    # add source files
    for line in makefile:
       
        if 'C_SOURCES =' in line:
            
            output.append(line)
            
            for source in src_files:
                source_line = source + ' \\\n'
                output.append(source_line)
                print_added(source + '\n')
                changes += 1

        elif 'C_INCLUDES =' in line:

            output.append(line)

            for include in inc_dirs:
                include_line = '-I' + include + ' \\\n'
                output.append(include_line)
                print_added(include + '\n')
                changes += 1

        else:
            output.append(line)
        
    # return output file data
    print_success('Done [{0} changes]'.format(changes))
    return output


###############################################################################
# RTOS port
###############################################################################

def fix_rtos_port(makefile):
    """Fix incorrectly generated ThreadX port
    """

    print_header('\nFixing ThreadX port')

    fixed = []
    changes = 0
    iar_only_sources = ['/src/tx_thread_interrupt_disable', \
                        '/src/tx_thread_interrupt_restore', \
                        '/src/tx_iar']
    
    for line in makefile:

        delete_line = False

        # remove IAR only source files
        for source in iar_only_sources:
            if source in line:
                delete_line = True
                changes += 1

        # replace IAR port paths with GNU port paths
        if '/iar/' in line:
            line = line.replace('/iar/', '/gnu/')

        # only keep lines not marked for deletion
        if not delete_line:
            fixed.append(line)
        else:
            print_deleted(line)
    
    # return the fixed file data
    print_success('Done [{0} changes]'.format(changes))
    return fixed

###############################################################################
# assembly .S -> .s
###############################################################################

def rename_asm_extensions():
    """Rename assembly files from .S to .s
    """
    print_header('\nRenaming .S files to .s')

    directories = ['Middlewares/ST/threadx/ports/cortex_m7/gnu/src', \
                   'Core/Src']
    changes = 0

    for directory in directories:

        files = [f for f in os.listdir(directory)]

        for file in files:

            if file[-2:] == '.S':

                new_file = file.replace('.S', '.s')
                old_path = directory + '/' + file
                new_path = directory + '/' + new_file

                os.rename(old_path, new_path)
                print_modified(file, new_file)

                changes += 1

    print_success('Done [{0} changes]'.format(changes))

if __name__ == '__main__':
    
    try:
        # read makefile data
        makefile = open('Makefile', 'r')
        makefile_data = makefile.readlines()
        makefile.close()
        
        # fix data
        makefile_data = fix_rtos_port(makefile_data)
        makefile_data = add_sufst_files(makefile_data)

        # fix files
        rename_asm_extensions()

        # write back to file
        makefile_out = open('Makefile', 'w')

        for line in makefile_data:
            makefile_out.write(line)

        makefile_out.close()
        print()


    except IOError:
        print('Error: Could not open Makefile')
        exit(1)
