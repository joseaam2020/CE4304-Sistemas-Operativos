#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Clasifica una imagen por color predominante (rojo, verde o azul)
// Retorna una cadena constante ("rojo", "verde", "azul") o NULL si falla
const char *clasificar(const char *archivo) {
  int width, height, channels;

  // 3 canales (RGB)
  unsigned char *img = stbi_load(archivo, &width, &height, &channels, 3);
  if (!img) {
    fprintf(stderr, "Error al cargar la imagen %s\n", archivo);
    return NULL;
  }

  uint64_t totalR = 0, totalG = 0, totalB = 0;
  int num_pixels = width * height;

  for (int i = 0; i < num_pixels * 3; i += 3) {
    totalR += img[i + 0];
    totalG += img[i + 1];
    totalB += img[i + 2];
  }

  stbi_image_free(img);

  if (totalR >= totalG && totalR >= totalB) {
    return "rojo";
  } else if (totalG >= totalR && totalG >= totalB) {
    return "verde";
  } else {
    return "azul";
  }
}

void ecualizar_histograma_rgb(const char *path) {
  int width, height, channels;

  // Cargar imagen en formato RGB (3 canales)
  unsigned char *img = stbi_load(path, &width, &height, &channels, 3);
  if (img == NULL) {
    printf("Error al cargar la imagen: %s\n", path);
    return;
  }

  printf("Imagen cargada: %s (%dx%d)\n", path, width, height);

  int num_pixels = width * height;
  int hist[3][256] = {{0}};
  int cdf[3][256] = {{0}};
  unsigned char lut[3][256];

  // Calcular histogramas
  for (int i = 0; i < num_pixels; i++) {
    hist[0][img[3 * i + 0]]++;
    hist[1][img[3 * i + 1]]++;
    hist[2][img[3 * i + 2]]++;
  }

  // Calcular CDFs y LUTs
  for (int c = 0; c < 3; c++) {
    cdf[c][0] = hist[c][0];
    for (int i = 1; i < 256; i++) {
      cdf[c][i] = cdf[c][i - 1] + hist[c][i];
    }

    // Buscar primer valor no cero
    int cdf_min = 0;
    for (int i = 0; i < 256; i++) {
      if (cdf[c][i] != 0) {
        cdf_min = cdf[c][i];
        break;
      }
    }

    // Crear LUT
    for (int i = 0; i < 256; i++) {
      lut[c][i] = (unsigned char)(((float)(cdf[c][i] - cdf_min) /
                                   (num_pixels - cdf_min)) *
                                      255.0f +
                                  0.5f);
    }
  }

  // Aplicar LUTs a la imagen
  for (int i = 0; i < num_pixels; i++) {
    img[3 * i + 0] = lut[0][img[3 * i + 0]];
    img[3 * i + 1] = lut[1][img[3 * i + 1]];
    img[3 * i + 2] = lut[2][img[3 * i + 2]];
  }

  // Crear nuevo nombre para la imagen ecualizada
  char output_path[512];
  snprintf(output_path, sizeof(output_path), "%s", path);

  // Guardar imagen en PNG
  if (!stbi_write_png(output_path, width, height, 3, img, width * 3)) {
    printf("Error al guardar la imagen ecualizada.\n");
  } else {
    printf("Imagen ecualizada guardada en: %s\n", output_path);
  }

  stbi_image_free(img);
}
