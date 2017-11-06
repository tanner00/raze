#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <curl/curl.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "gl.h"
#include "gl_math.h"
#include "dumb_string.h"
#include "tokenizer.h"

void get_request(CURL *curl, const char *url, dumb_string *recv) {
	size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
		dumb_string *ds = ((dumb_string **) userdata)[0];
		append_dumb_string(ds, ptr);
		return size * nmemb;
	}
	
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *[]) { recv });
	assert(curl_easy_perform(curl) == CURLE_OK);
}

// @TODO: !!!! Cache glyphs
// @TODO: assert strdup
// @TODO: safe_malloc, safe_realloc, NULL_FREE
// @TODO: Custom allocator
// @TODO: PUSH TO GIT NOW!!!!!!!!!!!!!!!!!!!!!!!!

int main(void) {

	curl_global_init(CURL_GLOBAL_ALL);
	
	CURL *curl = curl_easy_init();
	
	dumb_string s;
        init_dumb_string(&s, "", 256);
	get_request(curl, "https://razefiles.herokuapp.com/index", &s);
	array *tokens = tokenize(s.data, s.len);
	free_dumb_string(&s);

	for (size_t i = 0; i < tokens->size; ++i) {
	        token *a = ((token **) tokens->elems)[i];
		printf("%s\t%s\n", a->buf, token_type_strings[a->type]);
	}

	free_array(tokens);
	free(tokens);
	
	curl_global_cleanup();
	
	assert(glfwInit());
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	const int WINDOW_WIDTH  = 1024;
	const int WINDOW_HEIGHT = 768;
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Raze",
					      NULL, NULL);
	assert(window);
	
	const GLFWvidmode *const MODE = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwSetWindowPos(window, MODE->width / 2 - WINDOW_WIDTH / 2,
			 MODE->height / 2 - WINDOW_HEIGHT / 2);

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	assert(!glewInit());

	uint32_t VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	
#define X 0.5
	GLfloat vertices[] = {
		X,  X,  1.0, 1.0,
		X,  -X, 1.0, 0.0,
		-X, -X, 0.0, 0.0,
		-X, X,  0.0, 1.0
	};
        GLuint indices[] = {
		0, 1, 3,
		1, 2, 3,
	};
	GLuint VBO = make_buffer(GL_ARRAY_BUFFER, vertices, sizeof(vertices));
	GLuint IBO = make_buffer(GL_ELEMENT_ARRAY_BUFFER, indices, sizeof(indices));
	(void) IBO;
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
			      NULL);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
			      (void *) (2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	GLuint shader = make_shader_program("vs.glsl", "fs.glsl");
	GLuint text_shader = make_shader_program("vs.glsl", "text_fs.glsl");
	
	GLfloat projm[] = IDENTITY_MATRIX;
	ortho(projm, 0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);

	FT_Library ft;
	FT_Init_FreeType(&ft);

	FT_Face face;
	FT_New_Face(ft, "OpenSans-Regular.ttf", 0, &face);
	// @TODO: Invalid font sizes?
	FT_Set_Pixel_Sizes(face, 0, 72);

        size_t w, h;
	GLuint tex_id = generate_text_texture("The Web Sucks.", face, &w, &h);
	
	glClearColor(0.92, 0.92, 0.92, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glfwSwapInterval(-1);
	
	while (!glfwWindowShouldClose(window)) {
	        float s = glfwGetTime();
		glClear(GL_COLOR_BUFFER_BIT);

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			break;
		}

		glBindVertexArray(VAO);
		glUseProgram(text_shader);

		{		
			GLfloat modelm[] = IDENTITY_MATRIX;
			model_matrix(modelm, WINDOW_WIDTH / 2 - w / 2, WINDOW_HEIGHT / 2 - h / 2, w, h, 0);
		
			glBindTexture(GL_TEXTURE_2D, tex_id);
		
			glUniformMatrix4fv(glGetUniformLocation(shader, "proj"), 1, GL_FALSE, projm);
			glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, modelm);
		
			glUniform4f(glGetUniformLocation(shader, "color"), 0.1, 0.5, 0.8, 1.0);
		
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
		}
		
		glfwSwapBuffers(window);
		glfwPollEvents();
		float e = glfwGetTime();
		if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
			printf("Frame took: %fms\n", (e - s) * 1000);
		}
	}

	glDeleteTextures(1, &tex_id);
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shader);

	glfwTerminate();
	
	return 0;
}