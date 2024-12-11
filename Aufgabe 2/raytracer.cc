
#include "math.h"
#include "geometry.h"
#include "math.tcc"
#include <vector>
#include <algorithm>
#include <SDL2/SDL.h>


// Die folgenden Kommentare beschreiben Datenstrukturen und Funktionen
// Die Datenstrukturen und Funktionen die weiter hinten im Text beschrieben sind,
// haengen hoechstens von den vorhergehenden Datenstrukturen ab, aber nicht umgekehrt.


// verschiedene Materialdefinition, z.B. Mattes Schwarz, Mattes Rot, Reflektierendes Weiss, ...
// im wesentlichen Variablen, die mit Konstruktoraufrufen initialisiert werden.
#define MATTE_WHITE material{{0.8f, 0.8f, 0.8f}, 0.25f}
#define MATTE_RED material{{0.8f, 0.3f, 0.3f}, 0.25f}
#define MATTE_GREEN material{{0.3f, 0.8f, 0.3f}, 0.25f}
#define MATTE_BLUE material{{0.3f, 0.3f, 0.8f}, 0.25f}
#define MATTE_BLACK material{{0.2f, 0.2f, 0.2f}, 0.25f}

#define MIRROR material{{0.0f, 0.0f, 0.0f}, 0.25f, 1.0f, 0.9f, false}
#define GLASS material{{1.0f, 1.0f, 1.0f}, 0.25f, 1.52f, 0.9f, true}


// Die folgenden Werte zur konkreten Objekten, Lichtquellen und Funktionen, wie Lambertian-Shading
// oder die Suche nach einem Sehstrahl für das dem Augenpunkt am nächsten liegenden Objekte,
// können auch zusammen in eine Datenstruktur für die gesammte zu
// rendernde "Szene" zusammengefasst werden.


// Für die "Farbe" benötigt man nicht unbedingt eine eigene Datenstruktur.
// Sie kann als Vector3df implementiert werden mit Farbanteil von 0 bis 1.
// Vor Setzen eines Pixels auf eine bestimmte Farbe (z.B. 8-Bit-Farbtiefe),
// kann der Farbanteil mit 255 multipliziert  und der Nachkommaanteil verworfen werden.
using color = Vector3df;

// Das "Material" der Objektoberfläche mit ambienten, diffusem und reflektiven Farbanteil.
struct material{
    color col = {0.0f, 0.0f, 0.0f};
    float const_light = 0.3f;
    float density = 1.0f;
    float reflectivity = 0.0f;
    bool is_transmissive = false;
};

// Ein "Objekt", z.B. eine Kugel oder ein Dreieck, und dem zugehörigen Material der Oberfläche.
// Im Prinzip ein Wrapper-Objekt, das mindestens Material und geometrisches Objekt zusammenfasst.
// Kugel und Dreieck finden Sie in geometry.h/tcc
struct hitable{
    Sphere3df sphere = {{0.0f, 0.0f, 0.0f}, -1.0f};
    material mat = MATTE_BLACK;
};

// Punktförmige "Lichtquellen" können einfach als Vector3df implementiert werden mit weisser Farbe,
// bei farbigen Lichtquellen müssen die entsprechenden Daten in Objekt zusammengefaßt werden
// Bei mehreren Lichtquellen können diese in einen std::vector gespeichert werden.
struct light{
    Vector3df pos;
    float intensity;
};

bool hit_anything(Ray3df to_light, std::vector<hitable> &world){
    for (auto obj : world){
        float t = obj.sphere.intersects(to_light);
        if (0 < t && t < 1){
            return true;
        }
    }
    return false;
}


// Sie benötigen eine Implementierung von Lambertian-Shading, z.B. als Funktion
// Benötigte Werte können als Parameter übergeben werden, oder wenn diese Funktion eine Objektmethode eines
// Szene-Objekts ist, dann kann auf die Werte teilweise direkt zugegriffen werden.
// Bei mehreren Lichtquellen muss der resultierende diffuse Farbanteil durch die Anzahl Lichtquellen geteilt werden.
// Lambertian Shading-Funktion
color lambertian(hitable closest, Intersection_Context<float, 3> context, std::vector<hitable> &world, std::vector<light> &lights){
    // Überprüfen, ob es ein gültiges closest hitable-Objekt ist
    if (closest.sphere.radius != -1){
        // Initialisierung der Lichtintensität
        float total_light_intensity = 0.0f;

        // Iteration über alle Lichtquellen in der Szene
        for (const auto &light : lights){
            // Berechnung der Richtung zum Licht und Normalisierung
            Vector3df to_light_direction = light.pos - context.intersection;
            Vector3df to_light_normalized = to_light_direction;
            to_light_normalized.normalize();

            // Erzeugen eines Strahls zum Licht mit leichtem Offset vom Schnittpunkt (gegen Schattenakne)
            Ray3df to_light_ray = {context.intersection + 0.08f * to_light_normalized, 0.92f * to_light_direction};

            // Überprüfen, ob der Strahl das closest hitable-Objekt trifft
            if (!hit_anything(to_light_ray, world)){
                // Berechnung der Lichtintensität durch Lambertian Shading
                total_light_intensity += light.intensity * std::max(0.0f, context.normal * to_light_normalized);
            }
        }

        // Durchschnittliche Lichtintensität über alle Lichtquellen
        total_light_intensity /= lights.size();

        // Berechnung der finalen Farbe mit Lambertian Shading
        return (closest.mat.const_light + total_light_intensity) * closest.mat.col;
    }
    else{
        // Rückgabe von Schwarz für ungültige closest hitable-Objekte
        return {0.0f, 0.0f, 0.0f};
    }
}

float schlick_approximation(Vector3df inbound, Vector3df normal, hitable obj){
    // Berechnung des Winkels zwischen dem einfallenden Strahl und der Normalen
    float cos_x = -1.0f * (normal * inbound);

    // Berechnung der Reflektionskoeffizienten R0
    float r0 = (cos_x > 0) ? (1.0f - obj.mat.density) / (1.0f + obj.mat.density) : (obj.mat.density - 1.0f) / (obj.mat.density + 1.0f);
    r0 *= r0;

    // Überprüfung auf Brechung (n > 1.0)
    if (obj.mat.density > 1.0f){
        // Berechnung des Sinus des transmittierten Strahls
        float n = obj.mat.density;
        float sin_t2 = n * n * (1.0f - cos_x * cos_x);

        // Überprüfung auf Totalreflexion
        if (sin_t2 > 1.0f){
            return 1.0f;
        }

        // Aktualisierung des Kosinuswerts basierend auf dem berechneten Sinus
        cos_x = std::sqrt(1.0f - sin_t2);
    }

    // Berechnung des Werts x für die Schlick-Approximation
    float x = 1.0f - cos_x;

    // Schlick-Approximation für die Reflexionsintensität
    return r0 + (1.0f - r0) * x * x * x * x * x;
}

bool refract(Ray3df in, Ray3df &out, hitable object, Intersection_Context<float, 3> context){
    Vector3df normal = context.normal;
    float n1 = 1.0f; // Brechungsindex des Vakuums
    float n2 = object.mat.density; // Brechungsindex des Materials

    float cos_theta = -1.0f * (normal * in.direction);

    // Prüft, ob der Strahl aus dem Material herausgeht
    if (cos_theta < 0.0f){
        std::swap(n1, n2);
        cos_theta = -cos_theta;
        normal = -1.0f * normal;
    }
    float ratio_n1_n2 = n1 / n2;
    float sin_theta = ratio_n1_n2 * sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));
    // Prüft, ob Totalreflexion auftritt
    if (sin_theta > 1.0f){
        return false;
    }
    float cos_phi = sqrt(std::max(0.0f, 1.0f - sin_theta * sin_theta));
    // Berechnet die Richtung des gebrochenen Strahls
    out.direction = ratio_n1_n2 * in.direction + (ratio_n1_n2 * cos_theta - cos_phi) * normal;
    // Setzt den Ursprung des gebrochenen Strahls
    out.origin = context.intersection + 0.08f * out.direction;
    return true;
}


// Die rekursive raytracing-Methode. Am besten ab einer bestimmten Rekursionstiefe (z.B. als Parameter übergeben) abbrechen.
color ray_color(Ray3df ray, int depth, std::vector<hitable> &world, std::vector<light> &lights){
    // Überprüfe die Tiefe der Rekursion
    if (depth <= 0)
        return {0.0f, 0.0f, 0.0f};

// Für einen Sehstrahl aus allen Objekte, dasjenige finden, das dem Augenpunkt am nächsten liegt.
// Am besten einen Zeiger auf das Objekt zurückgeben. Wenn dieser nullptr ist, dann gibt es kein sichtbares Objekt.
    // Finde das nächstgelegene Objekt und seinen Treffpunkt
    hitable closest;
    float closest_t = std::numeric_limits<float>::max();
    Intersection_Context<float, 3> context;

    for (auto obj : world){
        Intersection_Context<float, 3> temp_context;
        if (obj.sphere.intersects(ray, temp_context) && temp_context.t < closest_t){
            closest = obj;
            closest_t = temp_context.t;
            context = temp_context;
        }
    }

    color col = {0, 0, 0};

    // Berechne den Schlick-Reflexionskoeffizienten
    float reflectivity = closest.mat.reflectivity;
    float transparency = closest.mat.is_transmissive ? 1.0f - reflectivity : 0.0f;

    if (reflectivity > 0.0f){
        // Reflektion
        Ray3df reflected_ray = {context.intersection + 0.08f * context.normal, 0.92f * ray.direction.get_reflective(context.normal)};
        color reflection = reflectivity * ray_color(reflected_ray, depth - 1, world, lights);

        if (transparency > 0.0f){
            // Transmission
            Ray3df refracted_ray;
            if (refract(ray, refracted_ray, closest, context)){
                color transmission = transparency * ray_color(refracted_ray, depth - 1, world, lights);
                col += 0.5f * (reflection + transmission);
            }
            else{
                // Totale innere Reflektion
                col += reflection;
            }
        }
        else{
            col += reflection;
        }
    }
    else if (transparency > 0.0f){
        // Nur Transmission
        Ray3df refracted_ray;
        if (refract(ray, refracted_ray, closest, context)){
            col += transparency * ray_color(refracted_ray, depth - 1, world, lights);
        }
    }
    else{
        // Lambertian-Shading
        col += lambertian(closest, context, world, lights);
    }

    return col;
}



// Eine "Kamera", die von einem Augenpunkt aus in eine Richtung senkrecht auf ein Rechteck (das Bild) zeigt.
// Für das Rechteck muss die Auflösung oder alternativ die Pixelbreite und -höhe bekannt sein.
// Für ein Pixel mit Bildkoordinate kann ein Sehstrahl erzeugt werden.

Vector3df get_ray_direction(int pos_v, int pos_u, int image_width, int image_height, float aspect_ratio, float focal_length, Vector3df cam_center){
    image_height = static_cast<int>(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    // Determine viewport dimensions.
    float viewport_height = 2.0;
    float viewport_width = viewport_height * (static_cast<double>(image_width) / image_height);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    Vector3df viewport_u{viewport_width, 0, 0};
    Vector3df viewport_v{0, -viewport_height, 0};

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    Vector3df pixel_delta_u = 1.0f/image_width * viewport_u;
    Vector3df pixel_delta_v = 1.0f/image_height * viewport_v;

    // Calculate the location of the upper left pixel.
    Vector3df viewport_upper_left = cam_center - Vector3df{0, 0, focal_length} - 0.5f * viewport_u - 0.5f * viewport_v;

    // Calculate the location of the current pixel.
    Vector3df pixel_position = viewport_upper_left + static_cast<float>(pos_u) * pixel_delta_u + static_cast<float>(pos_v) * pixel_delta_v;

    // Berechne die Richtung des Strahls vom Kamerastandpunkt zur Pixelposition
    Vector3df ray_direction = pixel_position - cam_center;
    ray_direction.normalize();

    return ray_direction;
}


// - für jeden einzelnen Pixel Farbe bestimmen
void render_sdl2(SDL_Renderer *pRenderer, int image_width, int image_height, int max_depth, std::vector<hitable> &world, std::vector<light> &lights, Vector3df cam_center, float focal_length, float aspect_ratio) {
    for (int v = 0; v < image_height; v++) {
        for (int u = 0; u < image_width; u++) {

            // Berechne die Richtung des Strahls für die aktuelle Pixelposition
            Vector3df ray_direction = get_ray_direction(v, u, image_width, image_height, aspect_ratio, focal_length, cam_center);

            // Erstelle einen Strahl für die aktuelle Pixelposition
            Ray3df ray = {cam_center, ray_direction};

            // Berechne die Farbe für den Strahl
            color pixel_color = ray_color(ray, max_depth, world, lights);

            // Setze die Renderfarbe basierend auf den RGB-Werten der berechneten Pixelfarbe
            SDL_SetRenderDrawColor(pRenderer, static_cast<Uint8>(pixel_color[0] * 255), static_cast<Uint8>(pixel_color[1] * 255), static_cast<Uint8>(pixel_color[2] * 255), 255);

            // Zeichne einen Punkt auf den Bildschirm an der aktuellen Position
            SDL_RenderDrawPoint(pRenderer, u, v);
        }
    }

}

// Ein "Bildschirm", der das Setzen eines Pixels kapselt
// Der Bildschirm hat eine Auflösung (Breite x Höhe)
// Kann zur Ausgabe einer PPM-Datei verwendet werden oder
// mit SDL2 implementiert werden.
SDL_Window *create_screen(int width, int height) {
    SDL_Window *window = SDL_CreateWindow( "Raytracing: Cornell-Box", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN );
    return window;
}




#ifdef _WIN32
#include <windows.h>
int main(void);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int CmdShow){
    return main();
}
#endif


int main(void){
    // Bildschirm erstellen
    // Kamera erstellen
    // Für jede Pixelkoordinate x,y
    //   Sehstrahl für x,y mit Kamera erzeugen
    //   Farbe mit raytracing-Methode bestimmen
    //   Beim Bildschirm die Farbe für Pixel x,y, setzten

    int image_width = 960;
    int max_depth = 10;

    Vector3df cam_center = {0.0f, 0.0f, 0.0f};
    float focal_length = 2.0f;
    float aspect_ratio = 16.0f / 9.0f;
    int image_height = image_width / aspect_ratio;


    // Die Cornelbox aufgebaut aus den Objekten
    // Am besten verwendet man hier einen std::vector< ... > von Objekten.
    std::vector<hitable> world;
    std::vector<light> lights;

    world.push_back({{{0, -100000, 0}, 99990}, MATTE_WHITE}); // Boden
    world.push_back({{{0, 100000, 0}, 99990}, MATTE_WHITE}); // Decke
    world.push_back({{{0, 0, -100000}, 99950}, MATTE_WHITE}); // Wand hinten
    //world.push_back({{{0, 0, 100000}, 99999}, MATTE_WHITE}); // Wand vorne
    world.push_back({{{-100000, 0, 0}, 99990}, MATTE_RED}); // Wand links
    world.push_back({{{100000, 0, 0}, 99990}, MATTE_GREEN}); // Wand rechts

    world.push_back({{{-5.0f, -6.0f, -24.5f}, 3.5f}, MATTE_BLUE});

    world.push_back({{{-3, -6.5f, -36.5f}, 4}, MIRROR});
    world.push_back({{{4, -6.5f, -32.0f}, 4}, GLASS});

    lights.push_back({{-1.0f, 8.0f, -40.0f}, 1.0f});



    SDL_Window *sdl_screen = create_screen(image_width, image_height);
    SDL_Renderer *renderer = SDL_CreateRenderer(sdl_screen, -1, SDL_RENDERER_ACCELERATED);

    render_sdl2(renderer, image_width, image_height, max_depth, world, lights, cam_center, focal_length, aspect_ratio);

    SDL_RenderPresent(renderer);

    SDL_Delay(10000);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(sdl_screen);
    SDL_Quit();

    return 0;
}

