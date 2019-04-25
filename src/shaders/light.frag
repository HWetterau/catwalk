R"zzz(
#version 330 core
uniform bool selected;
uniform vec4 color;
out vec4 fragment_color;
void main() {
   if(selected){
   	fragment_color = vec4(1.0, 1.0, 0.20, 1.0);
   } else {
      fragment_color = color;
   }

})zzz"