FILE(REMOVE_RECURSE
  "CMakeFiles/update_cmakelists_Compiler"
  "cmakelists_rebuilder.stamp"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/update_cmakelists_Compiler.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
