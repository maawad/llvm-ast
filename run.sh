example_dir="add"
example_cpp="add_two_numbers.cpp"

example_dir="hip_add"
example_cpp="add_two_numbers.hip"

function="addNumbers"
cmd="./build/ast_example
    -p examples/$example_dir/compile_commands.json\
    examples/$example_dir/$example_cpp"
    # --function=$function"
echo $cmd
$cmd > "$example_cpp.dot"
dot -Tpng "$example_cpp.dot" -o "$example_cpp.png"