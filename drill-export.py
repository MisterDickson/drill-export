import sys
import subprocess

if len(sys.argv) < 2:
    path = input("Pfad von der .kicad_pcb-Datei: ")
else:
    path = sys.argv[1]

exc_filename = path[0:path.rfind('.')] + '.exc' # to be exported as EAGLE-compatible

drl_filename = path[0:path.rfind('.')] + '.drl' # to be imported from KiCad

command = '"C:\Program Files\KiCad\\7.0\\bin\kicad-cli.exe" pcb export drill --drill-origin plot --excellon-zeros-format suppressleading -u in --excellon-min-header ' + path
subprocess.call(command, shell=True)

exc_file = open(exc_filename, "w")
drl_file = open(drl_filename, "r")

line = drl_file.readline()

# Skip the tool definitions
while line != 'T1\n':
    line = drl_file.readline()

while line:
    line = line.replace('X-', 'X')
    exc_file.write(line)
    line = drl_file.readline()