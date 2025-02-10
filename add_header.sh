#!/bin/bash

# Define the header you want to add
HEADER="Copyright (c) 2024 Chair for Chip Design for Embedded Computing,\n\
                   Technische Universitaet Braunschweig, Germany\n\
                   www.tu-braunschweig.de/en/eis\n\
\n\
Use of this source code is governed by an MIT-style\n\
license that can be found in the LICENSE file or at\n\
https://opensource.org/licenses/MIT\n"

# Declare an associative array for file extensions and specific filenames with their comment characters
declare -A COMMENT_CHARS
COMMENT_CHARS=( ["sh"]="#" ["py"]="#" ["c"]="/*" ["cpp"]="/*" ["h"]="/*" ["js"]="//" ["html"]="<!--" ["css"]="/*" ["Makefile"]="#" ["CMakeLists.txt"]="#" ["cmake"]="#")

# Function to add header to a file
add_header() {
    local file="$1"
    local filename="${file##*/}"
    local extension="${file##*.}"

    # Determine comment character
    local comment_char="${COMMENT_CHARS[$extension]}"
    if [[ -z "$comment_char" ]]; then
        comment_char="${COMMENT_CHARS[$filename]}"
    fi

    if [[ -z "$comment_char" ]]; then
        echo "Skipping $file: Unknown file extension or filename"
        return
    fi

    local header_content

    # Depending on the comment character, format the header
    case "$comment_char" in
        "/*")
            header_content="/*\n * $(echo -e "$HEADER" | sed 's/^/ * /')\n */"
            ;;
        "<!--")
            header_content="<!--\n$(echo -e "$HEADER" | sed 's/^/  /')\n-->"
            ;;
        *)
            header_content="$(echo -e "$HEADER" | sed "s/^/$comment_char /")"
            ;;
    esac

    # Check if the header already exists in the file
    if ! head -n 10 "$file" | grep -q "Technische Universitaet Braunschweig"; then
        # Prepend the header to the file
        { echo -e "$header_content"; cat "$file"; } > temp_file && mv temp_file "$file"
        echo "Header added to $file"
    else
        echo "Header already exists in $file"
    fi
}

# Use find to iterate over all desired files in the current directory and subdirectories, excluding 'venv' and 'build'
find . -type d \( -name "venv" -o -name "build" \) -prune -o -type f \( -name "*.*" -o -name "Makefile" -o -name "CMakeLists.txt" \) -print | while read -r file; do
    add_header "$file"
done

