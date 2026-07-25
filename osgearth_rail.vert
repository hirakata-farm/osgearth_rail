#version 430
uniform mat4 osg_ModelViewProjectionMatrix;
in vec4 osg_Vertex;
in vec3 osg_Normal;
in vec4 aColor;
out vec3 color;
void main()
{
	#float theta = dot(osg_Normal, normalize(vec3(0.0, 1.0, 0.0)));
	#color = vec3(0.0, 1.0, 0.0)*theta;
	color = aColor;
	gl_Position = osg_ModelViewProjectionMatrix*osg_Vertex;
}

