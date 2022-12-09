#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <iomanip>
#include <vector>
#include <cmath>
#include "ppm.h"
#include "mat.h"


#define MAX_RECUR_DEPTH 3
#define MIN_START_RAY 0.0001f

using namespace std;

struct Ray{
    vec4 origin;
    vec4 direction;
    int depth;

};

struct Sphere{
    string name;
    vec4 pos;
    vec3 scale;
    vec4 colour;
    float k_a, k_diff, k_spec, k_refl;
    int n;
    mat4 inverse;
};

struct Intersection{
    Ray ray;
    Sphere *sphere;
    vec4 normal;
    vec4 point;
    float distance;
    bool inside;
};

struct Light{
    string name;
    vec4 pos;
    vec4 light;
};

struct Parameters{
    float near, left, right, bottom, top;
    int rows, cols;
    vector<Sphere> sphereList;
    vector<Light> lightList;
    vec4 back;
    vec4 ambient;
    string output;
};

Parameters parameters;

Intersection calculateNearestIntersection(const Ray &ray) {
    Intersection hit;
    hit.ray = ray;
    hit.distance = -1;
    hit.inside = false;

    for (Sphere &sphere : parameters.sphereList) {
        float a = dot(sphere.inverse * ray.direction, sphere.inverse * ray.direction);
        float b = dot(sphere.inverse * (sphere.pos - ray.origin), sphere.inverse * ray.direction);
        float c = dot(sphere.inverse * (sphere.pos - ray.origin), sphere.inverse * (sphere.pos - ray.origin)) - 1;
        float Q;
        float discriminant = b * b - a * c;
        float solution1 = (b - sqrtf(discriminant)) / a;
        float solution2 = (b + sqrtf(discriminant)) / a; 

        bool insideCheck = false;

        if(discriminant > 0) {
            if (solution1 > solution2){
                Q = solution2;
            }
            else{
                Q = solution1;
            }
            if (Q <= MIN_START_RAY || (ray.depth == 0 && Q <= 1.0f)) {
                if (solution1 > solution2){
                    Q = solution1;
                }
                else{
                    Q = solution2;
                }
                insideCheck = true;
            }
        }
        else if(discriminant < 0) {
            continue;
        }

        if (Q <= MIN_START_RAY || (ray.depth == 0 && Q <= 1.0f)) {
            continue;
        }

        if ((hit.distance == -1 || Q < hit.distance)) {
            hit.distance = Q;
            hit.sphere = &sphere;
            hit.inside = insideCheck;
        }
    }

    if (hit.distance != -1) {
        hit.point = ray.origin + ray.direction * hit.distance;
        if (hit.inside) {
            vec4 normal = -(hit.point - hit.sphere->pos);
            mat4 inverseTranspose = transpose(hit.sphere->inverse);
            normal = inverseTranspose * hit.sphere->inverse * normal;
            normal.w = 0;
            hit.normal = normalize(normal);
        }
        else{
            vec4 normal = hit.point - hit.sphere->pos;
            mat4 inverseTranspose = transpose(hit.sphere->inverse);
            normal = inverseTranspose * hit.sphere->inverse * normal;
            normal.w = 0;
            hit.normal = normalize(normal);
        }
    }

    return hit;
}


vec4 traceRay(const Ray &ray) {
    Intersection hitRay = calculateNearestIntersection(ray);

    if (ray.depth >= MAX_RECUR_DEPTH) {
        return vec4(0, 0, 0, 0);
    }
    else if (hitRay.distance == -1 && ray.depth == 0) {
        return parameters.back;
    } else if (hitRay.distance == -1) {
        return vec4(0, 0, 0, 0);
    }
    else{
        vec4 diffuse, specular = vec4(0, 0, 0, 0);
        for (Light light : parameters.lightList) {
            Ray newLightRay;
            newLightRay.origin = hitRay.point;
            newLightRay.direction = normalize(light.pos - hitRay.point);

            Intersection lightRayHit = calculateNearestIntersection(newLightRay);
            vec4 H = normalize(newLightRay.direction - ray.direction);
            float sStrength = dot(hitRay.normal, H);
            float dStrength = dot(hitRay.normal, newLightRay.direction);

            if (lightRayHit.distance == -1) {
                if (dStrength > 0) {
                    diffuse += dStrength * light.light * hitRay.sphere->colour;
                    specular += powf(powf(sStrength, hitRay.sphere->n), 5) * light.light;
                }
            }
        }

        Ray reflectRay;
        reflectRay.origin = hitRay.point;
        reflectRay.direction = normalize(ray.direction - 2.0f * dot(hitRay.normal, ray.direction) * hitRay.normal);
        reflectRay.depth = ray.depth + 1;
        vec4 colour = hitRay.sphere->colour * hitRay.sphere->k_a * parameters.ambient +  diffuse * hitRay.sphere->k_diff + specular * hitRay.sphere->k_spec + traceRay(reflectRay) * hitRay.sphere->k_refl;
        colour.w = 1.0f;
       
        return colour;
    }
}

void parseFile (string fileName){
    if(fileName.substr(fileName.length() -4) != ".txt"){
        cout << "Filename was not of text form. Program terminated\n";
        return;
    }

    ifstream file;
    string queryLine, dataLine;
    file.open(fileName);
    if (!file){
        cout << "File could not be opened or did not exist\n";
        return;
    }

    while (getline(file, queryLine)){
        stringstream ss;
        string paramName; 
        ss << queryLine;
        ss >> paramName;

        if (paramName == "NEAR"){
            ss >> parameters.near;
        }
        else if(paramName == "LEFT"){
            ss >> parameters.left;
        }
        else if(paramName == "RIGHT"){
            ss >> parameters.right;
        }
        else if(paramName == "BOTTOM"){
            ss >> parameters.bottom;
        }
        else if(paramName == "TOP"){
            ss >> parameters.top;
        }
        else if(paramName == "RES"){
            ss >> parameters.rows;
            ss >> parameters.cols;
        }
        else if(paramName == "SPHERE"){
            Sphere newSphere;
            float x,y,z;
            ss >> newSphere.name;
            newSphere.pos.w = 1.0f;
            ss >> newSphere.pos.x;
            ss >> newSphere.pos.y;
            ss >> newSphere.pos.z;
            ss >> newSphere.scale.x;
            ss >> newSphere.scale.y;
            ss >> newSphere.scale.z;
            newSphere.colour.w = 1.0f;
            ss >> newSphere.colour.x;
            ss >> newSphere.colour.y;
            ss >> newSphere.colour.z;
            ss >> newSphere.k_a;
            ss >> newSphere.k_diff;
            ss >> newSphere.k_spec;
            ss >> newSphere.k_refl;
            ss >> newSphere.n;
            InvertMatrix(Scale(newSphere.scale), newSphere.inverse);
            parameters.sphereList.push_back(newSphere);
        }
        else if(paramName == "LIGHT"){
            Light newLight;
            ss >> newLight.name;
            newLight.pos.w = 1.0f;
            ss >> newLight.pos.x;
            ss >> newLight.pos.y;
            ss >> newLight.pos.z;
            newLight.light.w = 1.0f;
            ss >> newLight.light.x;
            ss >> newLight.light.y;
            ss >> newLight.light.z;
            parameters.lightList.push_back(newLight);
        }
        else if(paramName == "BACK"){
            parameters.back.w = 1.0f;
            ss >> parameters.back.x;
            ss >> parameters.back.y;
            ss >> parameters.back.z;
        }
        else if(paramName == "AMBIENT"){
            parameters.ambient.w = 1.0f;
            ss >> parameters.ambient.x;
            ss >> parameters.ambient.y;
            ss >> parameters.ambient.z;
        }
        else if(paramName == "OUTPUT"){
            string temp;
            ss >> temp;
            if(temp.size() > 20){
                cout << "Output name is greater than 20 characters, Program Termianting\n";
                return;
            }
            parameters.output = temp;
        }
        /*
        cout << parameters.sphereList.at(0).pos.w << "\n";
        cout << parameters.sphereList.at(0).pos.x << "\n";
        cout << parameters.sphereList.at(0).pos.y << "\n";
        cout << parameters.sphereList.at(0).pos.z << "\n";
        */
    }
    file.close();
}

int main(int argc, char *argv[]) {

    if (argc == 1){
        cout << "Not enough arguments\n Usage: ./raytracer <filename>\n";
        return -1;
    }
    parseFile(argv[1]);
    if (parameters.sphereList.size() > 15){
        cout << "Spherelist was too big, over the 15 size limit, program terminating" << "\n";
        return -1;
    }
    else if(parameters.lightList.size() > 10){
        cout << "Lightlist was too big, over the 10 size limit, program terminating" << "\n";
        return -1;
    }
    else{
        int Width = parameters.rows;	
        int Height = parameters.cols;
        unsigned char *pixels;
        pixels =  new unsigned char[Width * Height * 3];
        vector<vec4> allColours;
        allColours.resize((unsigned int) (Width * Height));

        for(int i = 0; i < Height; i++){
            for(int j = 0; j < Width; j++){
                float x = parameters.left + ((float) j / Width) * (parameters.right - parameters.left);
                float y = parameters.bottom + ((float) i / Height) * (parameters.top - parameters.bottom);
                Ray newRay;
                newRay.origin = vec4(0.0f, 0.0f, 0.0f, 1.0f);
                newRay.direction = vec4(x, y, -parameters.near, 0.0f);
                newRay.depth = 0;

                vec4 pixelColour = traceRay(newRay);
                allColours[(Height - i - 1) * Width + j] = pixelColour;
                /* only does half the image? so try putting it outside
                for (int k = 0; k < 3; k++){
                    if( ((float*) allColours[i * Width + j])[k] < 0 ) {
                        ((float*) allColours[i * Width + j])[k] *= -1; 
                    }
                    if( ((float*) allColours[i * Width + j])[k] > 1 ) { 
                        ((float*) allColours[i * Width + j])[k] = 1; 
                    } 
                    pixels[i * Width * 3 + j * 3 + k] = (unsigned char) (((float*) allColours[i * Width + j])[k] * 255.9f);
                }
                */
                /*
                Ray newRay = Ray()
                int c = j * Width + i;
                pixels[index] = c;
                pixels[index] = c;
                pixels[index] = c;
                index += 3;
                */
            }
        }
        for (int i = 0; i < Height; i++){
            for (int j = 0; j < Width; j++){
                for (int k = 0; k < 3; k++){
                    if( ((float*) allColours[i * Width + j])[k] < 0 ) {
                        ((float*) allColours[i * Width + j])[k] *= -1; 
                    }
                    if( ((float*) allColours[i * Width + j])[k] > 1 ) { 
                        ((float*) allColours[i * Width + j])[k] = 1; 
                    } 
                    pixels[i * Width * 3 + j * 3 + k] = (unsigned char) (((float*) allColours[i * Width + j])[k] * 255.9f);
                }
            }
        }
        ppm::save_imageP6(Width, Height, parameters.output.c_str(), pixels);
    }
}
