import sys

if sys.version_info[0] < 3:
    print("this script can only run on python 3");
else:
    import camera_node
    camera_node.main()
