###############################################################################
# @fn          check_size
# @brief       Prints the human-readable size of a file.
# @param[in]   $1  File path.
###############################################################################
check_size() {
    ls -lh -- "$1" | awk '{print $5}'
}

###############################################################################
# @fn          sed_inplace
# @brief       Cross-platform in-place sed.
# @param[in]   $@  sed arguments.
###############################################################################
sed_inplace() {
    if [[ "$(uname)" == "Darwin" ]]; then
        sed -i '' "$@"
    else
        sed -i "$@"
    fi
}

###############################################################################
# @fn          add_script_tag
# @brief       Wraps/unwraps JS files in <script> tags for minification.
# @param[in]   $1  File path.
# @param[in]   $2  true to add, false to remove tags.
###############################################################################
add_script_tag() {
    local file="$1"
    local add="$2"
    if [ "$add" = true ]; then
        add_script_tag "$file" false
        local tmp
        tmp=$(mktemp)
        { echo -n "<script>"; cat "$file"; echo -n "</script>"; } > "$tmp"
        mv "$tmp" "$file"
    else
        sed_inplace 's|<script>||g; s|</script>||g' "$file"
    fi
}

###############################################################################
# @fn          add_ini_variables
# @brief       Replaces {KEY} in file with -DKEY=VALUE from platformio.ini.
# @param[in]   $1  File path.
###############################################################################
add_ini_variables() {
    local file="$1"
    local tmp
    tmp=$(mktemp)
    grep '^[[:space:]]*-D' "$2/../platformio.ini" | while read -r line; do
        if [[ "$line" =~ -D([A-Za-z0-9_]+)=(.+) ]]; then
            local key="${BASH_REMATCH[1]}"
            local raw="${BASH_REMATCH[2]}"
            raw="${raw//\\\"/}"
            raw="${raw//\"/}"
            local raw_escaped
            raw_escaped=$(printf '%s' "$raw" | sed -e 's/[\/&|]/\\&/g')
            printf "%s\t%s\n" "$key" "$raw_escaped"
        fi
    done > "$tmp.kv"
    cp "$file" "$tmp.out"
    while IFS=$'\t' read -r key value; do
        sed_inplace "s|{$key}|$value|g" "$tmp.out"
    done < "$tmp.kv"
    mv "$tmp.out" "$file"
    rm -f "$tmp.kv"
}

###############################################################################
# @fn          compile_file
# @brief       Minifies and processes a web file for upload.
# @param[in]   $1  Source file path.
###############################################################################

OUT="$2/../data/$(basename "$1")"
if [[ "$1" == *.js ]]; then
    add_script_tag "$1" true
fi
html-minifier \
    --collapse-whitespace \
    --remove-comments \
    --remove-optional-tags \
    --remove-redundant-attributes \
    --remove-script-type-attributes \
    --remove-tag-whitespace \
    --use-short-doctype \
    --minify-css true \
    --minify-js true \
    "$1" -o "$OUT"
if [[ "$1" == *.js ]]; then
    add_script_tag "$1" false
    add_script_tag "$OUT" false
fi
add_ini_variables "$OUT" "$2"
echo "> Compiled: $(check_size "$1") → $(check_size "$OUT"): $(basename "$1")"
