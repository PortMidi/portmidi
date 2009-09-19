# awk script to convert CMake-generated xcode project into a stand-alone project file
#
# Roger Dannenberg, September 2009
#
# the result does not call CMake (so you do not need to install CMake to use it)
# the result uses relative path names (so you can install the project on any machine and use it)
#

# remove the base path from a string to make paths relative
#
function make_relative(s)
{
    gsub(base_path_slash, "./", s)
    return s
}


BEGIN {
    state = "normal";
    # change the following path to the path in which the CMakeLists.txt file resides
    base_path = "/Users/rbd/portmedia/portmidi";
    base_path_slash = (base_path "/")
}
# this case removes CMake script phases from project
state == "build_phases" {
    # print "IN BUILD PHASES " index($0, "/* CMake ReRun") "---" index($0, " CMake ")
    if (index($0, "/* CMake ") > 0) { 
        #print "DELETE: " $0; 
        next # skip the output
    } else { print $0 };

    if (index($0, ");") > 0) { 
        state = "normal";
    };
    next
}

# this case removes the script phases
state == "shell_script" {
    if (index($0, "};") > 0) {
        state = "normal";
    };
    next
}

# this is the normal case, not in buildPhases list
state == "normal" {
    # print "NOT IN BUILD PHASES"
    # take out all the absolute paths
    gsub(base_path_slash, "", $0); 
    # change projectRoot to empty string: ""
    if (index($0, "projectRoot = /") > 0) { 
        sub(base_path, "\"\"", $0) 
    }
    # change <absolute> file references to SOURCE_ROOT:
    if (index($0, "sourceTree = \"<absolute>\";") > 0) {
        sub("\"<absolute>\"", "SOURCE_ROOT", $0)
    };
    if (index($0, "buildPhases = (") > 0) {
        state = "build_phases"
    } else if (index($0, "/* CMake ReRun */ = {") > 0) {
        state = "shell_script";
        next # do not print this line
    };
    # remove any line with CMakeLists.txt -- it's either a file or file reference
    if (index($0, "CMakeLists.txt") > 0) {
        next # do not print this line
    };
    print $0;
    next
}

