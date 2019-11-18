if [ "$#" -ne 4 ]; then
    echo "Usage: ./dump.sh <input> <name> <output.c> <output.h>"
    exit
fi

input_filename="$1"
name="$2"
source_filename="$3"
header_filename="$4"

{
  printf "%s" "char ${name}_data[] = {";
  hexdump -v -e '1/1 "0x%02X,"' < "$input_filename";
  printf "0x00%s\n%s\n" "};" "unsigned ${name}_size = sizeof(${name}_data) - 1;";
} > "$source_filename"

{
  echo "extern char ${name}_data[];";
  echo "extern unsigned ${name}_size;";
} > "$header_filename"
