#version 430 core
precision mediump float;
precision lowp sampler2D;
//@OGLU@Stage("VERT", "FRAG")

const float oglu_PI = 3.1415926f;


//@OGLU@Mapping(LightMat, "matLight")
uniform mat4 matLight;
//@OGLU@Mapping(ModelMat, "matModel")
uniform mat4 matModel;


////////////////
#ifdef OGLU_VERT

//@OGLU@Mapping(VertPos, "vertPos")
in vec3 vertPos;

void main()
{
    gl_Position = matLight * matModel * vec4(vertPos, 1.0f);
}

#endif

#ifdef OGLU_FRAG

void main() 
{
}

#endif