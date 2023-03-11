Make script resources from .chai files in the main project:

 for f in ../../../../scripts/*.chai ; do cp $f ./$(basename $f .chai).tres ; done
