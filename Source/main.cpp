#include <iostream>
#include "parser.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

//////-------- Global Variables -------/////////

GLuint gpuVertexBuffer;
GLuint gpuNormalBuffer;
GLuint gpuIndexBuffer;

// Sample usage for reading an XML scene file
using namespace parser;
parser::Scene scene;
static GLFWwindow* win = NULL;

Vec3f add(const Vec3f &a,const Vec3f &b){
    Vec3f res;
    res.x = a.x + b.x;
    res.y = a.y + b.y;
    res.z = a.z + b.z;
    return res;

}
Vec3f sub(const Vec3f &a,const Vec3f &b){
    Vec3f res;
    res.x = a.x - b.x;
    res.y = a.y - b.y;
    res.z = a.z - b.z;
    return res;
}

Vec3f normalized(const Vec3f &a){
    Vec3f res;
    float length = sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
    res.x = a.x/length;
    res.y = a.y/length;
    res.z = a.z/length;
    return res;
}
Vec3f crossP(const Vec3f &a,const Vec3f &b){
    Vec3f res;
    res.x = a.y*b.z - a.z*b.y;
    res.y = a.z*b.x - a.x*b.z;
    res.z = a.x*b.y - a.y*b.x;
    return res;
}
static void errorCallback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}
int findIndiceArrayLength(const Scene scene){
    int res = 0;
    for(int i=0;i<scene.meshes.size();i++){
        res += scene.meshes[i].faces.size();
    }
    return res;
}

void calcNormals(Scene &scene){
    for(int i=0;i<scene.meshes.size();i++){
        Mesh mesh = scene.meshes[i];
        for(int j=0;j<mesh.faces.size();j++){
            Face face = mesh.faces[j];
            Vec3f a,b,c,bsa,csa,normal;
            a = scene.vertex_data[face.v0_id-1];
            b = scene.vertex_data[face.v1_id-1];
            c = scene.vertex_data[face.v2_id-1];
            bsa = sub(b,a);
            csa = sub(c,a);
            normal = normalized(crossP(bsa,csa));
            scene.normal_data[face.v0_id-1] += normal;
            scene.normal_data[face.v1_id-1] += normal;
            scene.normal_data[face.v1_id-1] += normal;
        }
    }
    for(int i=0;i<scene.normal_data.size();i++){
        scene.normal_data[i] = normalized(scene.normal_data[i]);
    }
}
void sendObjects(const Scene &scene,GLuint &vertex_buffer,GLuint &index_buffer,GLuint &normal_buffer){
    
    int totalIndices = findIndiceArrayLength(scene);
    GLfloat *vertices = new GLfloat[scene.vertex_data.size()*3];
    GLuint *indices = new GLuint[totalIndices*3];
    GLfloat *normals = new GLfloat[scene.vertex_data.size()*3];
    int c = 0;
    for(int i=0;i<scene.vertex_data.size();i++){
        normals[c] = scene.normal_data[i].x;
        vertices[c++] = scene.vertex_data[i].x;
        normals[c] = scene.normal_data[i].y;
        vertices[c++] = scene.vertex_data[i].y;
        normals[c] = scene.normal_data[i].z;
        vertices[c++] = scene.vertex_data[i].z;
    }
    c = 0;
    for(int i=0;i<scene.meshes.size();i++){
        Mesh mesh = scene.meshes[i];
        for(int j=0;j<mesh.faces.size();j++){
            indices[c++] = mesh.faces[j].v0_id-1;
            indices[c++] = mesh.faces[j].v1_id-1;
            indices[c++] = mesh.faces[j].v2_id-1;
        }
    }
    glGenBuffers(1,&vertex_buffer);
    glGenBuffers(1,&index_buffer);
    glGenBuffers(1,&normal_buffer);
    glBindBuffer(GL_ARRAY_BUFFER,vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*scene.vertex_data.size()*3,vertices,GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,normal_buffer);
    glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*scene.vertex_data.size()*3,normals,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(GLuint)*totalIndices*3,&indices[0],GL_STATIC_DRAW);
    glEnable(GL_DEPTH_TEST);
}

void setCamera(const Camera &cam){
    glViewport(0,0,cam.image_width,cam.image_height);
    Vec3f pos,gaze,up;
    pos = cam.position;
    gaze = cam.gaze;
    up = cam.up;
    Vec4f near = cam.near_plane;
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(pos.x,pos.y,pos.z,pos.x+gaze.x,pos.y+gaze.y,pos.z+gaze.z,up.x,up.y,up.z);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(near.x,near.y,near.z,near.w,cam.near_distance,cam.far_distance);
    
}
void lookat(const Camera &cam){
    Vec3f pos,gaze,up;
    pos = cam.position;
    gaze = cam.gaze;
    up = cam.up;
    Vec4f near = cam.near_plane;
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(pos.x,pos.y,pos.z,pos.x+gaze.x,pos.y+gaze.y,pos.z+gaze.z,up.x,up.y,up.z);

}
void createTransformation(const Camera &cam,const Mesh &mesh){
    int numT = mesh.transformations.size();
    glMatrixMode(GL_MODELVIEW);
    //lookat(cam);
    Vec3f vec3;
    Vec4f vec4;
    for(int i=mesh.transformations.size()-1;i>=0;i--){
        Transformation current = mesh.transformations[i];
        if(current.transformation_type=="Translation"){
            vec3 = scene.translations[current.id-1];
            glTranslated(vec3.x,vec3.y,vec3.z);
        }
        else if(current.transformation_type=="Rotation"){
            vec4 = scene.rotations[current.id-1];
            glRotated(vec4.x,vec4.y,vec4.z,vec4.w);
        }
        else if(current.transformation_type=="Scaling"){
            vec3 = scene.scalings[current.id-1];
            glScaled(vec3.x,vec3.y,vec3.z);
        }
    }

}

void draw(const Scene &scene,GLuint &vertex_buffer,GLuint &index_buffer,GLuint &normal_buffer,int *&numTriangles){
    Camera cam = scene.camera;
    int offset = 0;
    //glShadeModel(GL_SMOOTH);
    glEnableVertexAttribArray(0);
    setCamera(cam);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
    glBindBuffer(GL_ARRAY_BUFFER,normal_buffer);
    glNormalPointer(GL_FLOAT,0,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    for(int i=0;i<scene.meshes.size();i++){
        Mesh mesh = scene.meshes[i];
        Material mat = scene.materials[mesh.material_id-1];
        GLfloat ambColor[4] = {mat.ambient.x,mat.ambient.y,mat.ambient.z,1.0f};
        GLfloat difColor[4] = {mat.diffuse.x,mat.diffuse.y,mat.diffuse.z,1.0f};
        GLfloat specColor[4] = {mat.specular.x,mat.specular.y,mat.specular.z,1.0f};
        GLfloat specExp[1] = {mat.phong_exponent};
        if(mesh.mesh_type=="Solid"){
            glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        }
        else{
            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
        }
        glMaterialfv(GL_FRONT,GL_AMBIENT,ambColor);
        glMaterialfv(GL_FRONT,GL_DIFFUSE,difColor);
        glMaterialfv(GL_FRONT,GL_SPECULAR,specColor);
        glMaterialfv(GL_FRONT,GL_SHININESS,specExp);

        lookat(cam);
        createTransformation(cam,mesh);
        
        glDrawElements(GL_TRIANGLES,numTriangles[i]*3,GL_UNSIGNED_INT,(void*)(sizeof(GLuint)*offset));
        offset += numTriangles[i]*3;
        
        
    }
    glDisableVertexAttribArray(0);
}

void enableLights(const Scene &scene){
    glEnable(GL_LIGHTING);
    Vec3f ambient = scene.ambient_light;
    GLfloat ambiento[] = {ambient.x,ambient.y,ambient.z,1.0f};
    GLfloat a0[] = {0,0,0,1.0f};
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0,GL_AMBIENT,ambiento);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,a0);
    glLightfv(GL_LIGHT0,GL_SPECULAR,a0);
    
    for(int i=0;i<scene.point_lights.size();i++){
        glEnable(GL_LIGHT1+i);
        PointLight light = scene.point_lights[i];
        Vec3f posv = light.position;
        Vec3f colv = light.intensity;
        GLfloat col[] ={colv.x,colv.y,colv.z,1.0f};
        GLfloat pos[] = {posv.x,posv.y,posv.z,1.0f};
        GLfloat intensity[] = {light.intensity.x,light.intensity.y,light.intensity.z,1.0f};
        glLightfv(GL_LIGHT1+i,GL_POSITION,pos);
        glLightfv(GL_LIGHT1+i,GL_AMBIENT,a0);
        glLightfv(GL_LIGHT1+i,GL_DIFFUSE,col);
        glLightfv(GL_LIGHT1+i,GL_SPECULAR,col);
    }
}

double lastTime;
int nbFrames;

void showFPS(GLFWwindow *pWindow)
{
    // Measure speed
     double currentTime = glfwGetTime();
     double delta = currentTime - lastTime;
	 char ss[500] = {};
     nbFrames++;
     if ( delta >= 1.0 ){ // If last cout was more than 1 sec ago
         //cout << 1000.0/double(nbFrames) << endl;

         double fps = ((double)(nbFrames)) / delta;

         sprintf(ss,"CENG477 - HW3 [%lf FPS]",fps);

         glfwSetWindowTitle(pWindow, ss);

         nbFrames = 0;
         lastTime = currentTime;
     }
}

int main(int argc, char* argv[]) {
    scene.loadFromXml(argv[1]);

    glfwSetErrorCallback(errorCallback);

   
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    win = glfwCreateWindow(scene.camera.image_width, scene.camera.image_height, "CENG477 - HW3", NULL, NULL);
    if (!win) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    glfwMakeContextCurrent(win);

    GLenum err = glewInit();
   
    if (err != GLEW_OK) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    glClearColor(scene.background_color.x,scene.background_color.y,scene.background_color.z,0);
    glEnable(GL_DEPTH_TEST);
    glfwSetKeyCallback(win, keyCallback);
    

    //////////////////////

    if(scene.culling_enabled){
        glEnable(GL_CULL_FACE);
        if(scene.culling_face){
            glCullFace(GL_FRONT);
        }
        else{
            glCullFace(GL_BACK);
        }
    }
    Camera cam = scene.camera;
    GLuint vertex_buffer,index_buffer,normal_buffer;
    int totalIndices = findIndiceArrayLength(scene);
    int numMeshes = scene.meshes.size();
    int *numTriangles = new int[numMeshes];
    for(int i=0;i<numMeshes;i++){
        numTriangles[i] = scene.meshes[i].faces.size();
    }

    calcNormals(scene);
    sendObjects(scene,vertex_buffer,index_buffer,normal_buffer);
    enableLights(scene);
    glShadeModel(GL_SMOOTH);   
    setCamera(cam);
    
    glEnableClientState(GL_NORMAL_ARRAY);
    glDisable(GL_DEPTH_TEST);

   
    while(!glfwWindowShouldClose(win)) {
        draw(scene,vertex_buffer,index_buffer,normal_buffer,numTriangles);
        
        showFPS(win);

        glfwPollEvents();
        
        glfwSwapBuffers(win);
        
        
    }
    
    glfwDestroyWindow(win);
    glfwTerminate();

    exit(EXIT_SUCCESS);

    return 0;
}
