# list_split
# Split a list into values matching or not matching a given expression.
macro(list_split matching non_matching orig expression)
  foreach(val ${orig})
    if(${val} MATCHES ${expression})
      list(APPEND ${matching} ${val})
    else()
      list(APPEND ${non_matching} ${val})
    endif()
  endforeach()
endmacro()

