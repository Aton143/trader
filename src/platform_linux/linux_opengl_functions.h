GL_FUNC(glDebugMessageControl, void, (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled));
GL_FUNC(glDebugMessageCallback, void, (GLDEBUGPROC callback, const void *userParam))

GL_FUNC(glGenVertexArrays,    void, (GLsizei n, GLuint *arrays))
GL_FUNC(glBindVertexArray,    void, (GLuint array))

GL_FUNC(glDeleteVertexArrays, void, (GLsizei n, const GLuint *arrays))

GL_FUNC(glGenBuffers, void, (GLsizei n, GLuint *buffers))
GL_FUNC(glBindBuffer, void, (GLenum target, GLuint buffer))
GL_FUNC(glBufferData, void, (GLenum target, GLsizeiptr size, const void *data, GLenum usage))
GL_FUNC(glBufferSubData, void, (GLenum target, GLsizeiptr offset, GLsizeiptr size, const void *data))

GL_FUNC(glCreateShader, GLuint, (GLenum type))
GL_FUNC(glShaderSource, void, (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length))
GL_FUNC(glCompileShader, void, (GLuint shader))
GL_FUNC(glGetShaderiv, void, (GLuint shader, GLenum pname, GLint *params))
GL_FUNC(glGetShaderInfoLog, void, (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog))
GL_FUNC(glDeleteShader, void, (GLuint shader))
GL_FUNC(glCreateProgram, GLuint, (void))
GL_FUNC(glAttachShader, void, (GLuint program, GLuint shader))
GL_FUNC(glLinkProgram, void, (GLuint program))
GL_FUNC(glUseProgram, void, (GLuint program))
GL_FUNC(glGetProgramiv, void, (GLuint program, GLenum pname, GLint *params))
GL_FUNC(glGetProgramInfoLog, void, (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog))
GL_FUNC(glDeleteProgram, void, (GLuint program))
GL_FUNC(glValidateProgram, void, (GLuint program))

GL_FUNC(glGetUniformLocation, GLint, (GLuint program, const GLchar *name))
GL_FUNC(glGetAttribLocation, GLint, (GLuint program, const GLchar *name))

GL_FUNC(glBindAttribLocation, void, (GLuint program, GLuint index, const GLchar *name))
GL_FUNC(glDisableVertexAttribArray, void, (GLuint index))
GL_FUNC(glEnableVertexAttribArray, void, (GLuint index))

GL_FUNC(glVertexAttribPointer, void, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer))

GL_FUNC(glVertexAttribIPointer, void, (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer))

GL_FUNC(glUniform1f, void, (GLint location, GLfloat v0))
GL_FUNC(glUniform2f, void, (GLint location, GLfloat v0, GLfloat v1))
GL_FUNC(glUniform3f, void, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2))
GL_FUNC(glUniform4f, void, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3))
GL_FUNC(glUniform1i, void, (GLint location, GLint v0))
GL_FUNC(glUniform2i, void, (GLint location, GLint v0, GLint v1))
GL_FUNC(glUniform3i, void, (GLint location, GLint v0, GLint v1, GLint v2))
GL_FUNC(glUniform4i, void, (GLint location, GLint v0, GLint v1, GLint v2, GLint v3))
GL_FUNC(glUniform1fv, void, (GLint location, GLsizei count, const GLfloat *value))
GL_FUNC(glUniform2fv, void, (GLint location, GLsizei count, const GLfloat *value))
GL_FUNC(glUniform3fv, void, (GLint location, GLsizei count, const GLfloat *value))
GL_FUNC(glUniform4fv, void, (GLint location, GLsizei count, const GLfloat *value))
GL_FUNC(glUniform1iv, void, (GLint location, GLsizei count, const GLint *value))
GL_FUNC(glUniform2iv, void, (GLint location, GLsizei count, const GLint *value))
GL_FUNC(glUniform3iv, void, (GLint location, GLsizei count, const GLint *value))
GL_FUNC(glUniform4iv, void, (GLint location, GLsizei count, const GLint *value))
GL_FUNC(glUniformMatrix2fv, void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))
GL_FUNC(glUniformMatrix3fv, void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))
GL_FUNC(glUniformMatrix4fv, void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))

GL_FUNC(glGenFramebuffers, void, (GLsizei n, GLuint *framebuffers))
GL_FUNC(glBindFramebuffer, void, (GLenum target, GLuint framebuffer))
GL_FUNC(glCheckFramebufferStatus, GLenum, (GLenum target))
GL_FUNC(glFramebufferTexture1D, void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level))
GL_FUNC(glFramebufferTexture2D, void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level))
GL_FUNC(glFramebufferTexture3D, void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset))

GL_FUNC(glBlitFramebuffer, void, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter))

GL_FUNC(glTexImage2DMultisample, void, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations))

GL_FUNC(glGenerateMipmap, void, (GLenum target))
GL_FUNC(glBlendFuncSeparate, void, (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha))
GL_FUNC(glDrawArraysInstanced, void, (GLenum mode, GLint first, GLint count, GLsizei instancecount))
GL_FUNC(glMapBuffer, void *, (GLenum target, GLenum access))
GL_FUNC(glUnmapBuffer, GLboolean, (GLenum target))
GL_FUNC(glVertexAttribDivisor, void, (GLuint index, GLuint divisor))
GL_FUNC(glDrawArraysInstancedBaseInstance, void, (GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance))
 
GL_FUNC(glXSwapIntervalEXT, void, (Display *dpy, GLXDrawable drawable, int interval))
GL_FUNC(glXSwapIntervalMESA, int, (unsigned int interval))
GL_FUNC(glXGetSwapIntervalMESA, int, (void))
GL_FUNC(glXSwapIntervalSGI, int, (int interval))

#undef GL_FUNC
