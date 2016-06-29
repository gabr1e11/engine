/**
 * @class	OpenGLRenderer
 * @brief	OpenGL based 3D renderer
 *
 * @author	Roberto Cano (http://www.robertocano.es)
 */
#include "OpenGL.h"
#include "Logging.hpp"
#include "ModelLoaders.hpp"
#include "OpenGLLightingShader.hpp"
#include "OpenGLModel3D.hpp"
#include "OpenGLRenderer.hpp"
#include "OpenGLShader.hpp"

using namespace Logging;

void OpenGLRenderer::init()
{
    __(glClearColor(0.0, 0.0, 0.0, 1.0));
    __(glEnable(GL_DEPTH_TEST));
    __(glEnable(GL_CULL_FACE));
    __(glEnable(GL_BLEND));
    __(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    /* There seems to be a bug with sRGB default framebuffer, as
     * the buffer is always linear, but if we enable this the
     * drivers are doing the linear->sRGB conversion anyway!
     * So don't enable it for now, we will have to compensate
     * in the shaders */
    //__( glEnable(GL_FRAMEBUFFER_SRGB) );
    __(glCullFace(GL_BACK));
    __(glDisable(GL_DITHER));
    __(glDisable(GL_LINE_SMOOTH));
    __(glDisable(GL_POLYGON_SMOOTH));
    __(glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE));
#define GL_MULTISAMPLE_ARB 0x809D
    __(glDisable(GL_MULTISAMPLE_ARB));
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    __(glDepthRangef(0.1f, 1000.0f));

    /* Generate a fake texture to fullfill GLSL 3.3 requirement
     * on some cards that need all samplers to be bound to a valid
     * texture, even if they are not used */
    __(glGenTextures(1, &_dummyTexture));

    /* TODO: Once we use our own format, this should not be needed */
    __(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    __(glBindTexture(GL_TEXTURE_2D, _dummyTexture));
    {
        __(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        __(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        __(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
        __(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    }
    __(glBindTexture(GL_TEXTURE_2D, 0));
}

const char *OpenGLRenderer::getName() { return (const char *)glGetString(GL_RENDERER); }
const char *OpenGLRenderer::getVersion() { return (const char *)glGetString(GL_VERSION); }
const char *OpenGLRenderer::getVendor() { return (const char *)glGetString(GL_VENDOR); }
const char *OpenGLRenderer::getShaderVersion() { return (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION); }
Shader *OpenGLRenderer::newShader(void) { return new OpenGLShader(); }
Model3D *OpenGLRenderer::loadModelOBJ(const std::string &modelName)
{
    OpenGLModel3D *model = new OpenGLModel3D();
    if (model == NULL) {
        log("ERROR allocating memory for OpenGLModel3D\n");
        return NULL;
    }

    if (ModelLoaders::LoadOBJModel(*model, modelName) == false) {
        log("ERROR loading model %s into an OpenGLModel3D\n", modelName.c_str());
        delete model;
        return NULL;
    }

    if (model->prepare() == false) {
        log("ERROR preparing OpenGL arrays for model %s\n", modelName.c_str());
        delete model;
        return NULL;
    }

    return model;
}

Model3D *OpenGLRenderer::prepareModel(const Model3D &source)
{
    OpenGLModel3D *model = new OpenGLModel3D();
    if (model == NULL) {
        log("ERROR allocating memory for OpenGLModel3D\n");
        return NULL;
    }

    *((Model3D *)model) = source;

    if (model->prepare() == false) {
        log("ERROR preparing OpenGL arrays for model copied from other model\n");
        delete model;
        return NULL;
    }

    return model;
}

bool OpenGLRenderer::renderModel3D(Model3D &model3D, Camera &camera, LightingShader &shader, DirectLight *sun,
                                   std::vector<PointLight *> &pointLights, std::vector<SpotLight *> &spotLights, float ambientK,
                                   RenderTarget &renderTarget, bool disableDepth)
{
    glm::mat4 biasMatrix(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0);
    GLuint textureUnit = 0;
    GLuint dummyTextureUnit = 0;

    /* Calculate MVP matrix */
    glm::mat4 MVP = camera.getPerspectiveMatrix() * camera.getViewMatrix() * model3D.getModelMatrix();

    /* Calculate normal matrix */
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model3D.getModelMatrix())));

    /* Cast the model into an internal type */
    OpenGLModel3D &glObject = dynamic_cast<OpenGLModel3D &>(model3D);

    if (disableDepth) {
        glDisable(GL_DEPTH_TEST);
    } else {
        glEnable(GL_DEPTH_TEST);
    }

    if (getWireframeMode() == true) {
        __(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
        __(glEnable(GL_LINE_SMOOTH));
        __(glDisable(GL_CULL_FACE));
    } else {
        __(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
        __(glDisable(GL_LINE_SMOOTH));
        __(glEnable(GL_CULL_FACE));
    }

    /* Bind the render target */
    renderTarget.bind();
    {
        __(glEnable(GL_MULTISAMPLE));

        /* Bind program to upload the uniform */
        shader.attach();

        /* Send our transformation to the currently bound shader, in the "MVP" uniform */
        shader.setUniformMat4("u_MVPMatrix", &MVP);
        shader.setUniformMat4("u_viewMatrix", &camera.getViewMatrix());
        shader.setUniformMat4("u_modelMatrix", &model3D.getModelMatrix());
        shader.setUniformMat3("u_normalMatrix", &normalMatrix);
        shader.setUniformTexture2D("u_diffuseMap", textureUnit++);
        shader.setUniformFloat("u_ambientK", ambientK);

        /* Activate and bind unit 0 for the dummy texture */
        dummyTextureUnit = textureUnit++;
        __(glActiveTexture(GL_TEXTURE0 + dummyTextureUnit));
        __(glBindTexture(GL_TEXTURE_2D, _dummyTexture));

        /* Set the sun light */
        if (sun != NULL) {
            glm::mat4 shadowMVP = sun->getProjectionMatrix() * sun->getViewMatrix() * model3D.getModelMatrix();
            shadowMVP = biasMatrix * shadowMVP;

            shader.setDirectLight(*sun);
            shader.setUniformUint("u_numDirectLights", 1);

            /* TODO: This has to be set in a matrix array */
            shader.setUniformMat4("u_shadowMVPDirectLight", &shadowMVP);
            shader.setUniformTexture2D("u_shadowMapDirectLight", textureUnit);

            __(glActiveTexture(GL_TEXTURE0 + textureUnit));
            sun->getShadowMap()->bindDepth();

            textureUnit++;
        } else {
            shader.setUniformUint("u_numDirectLights", 0);
            shader.setUniformTexture2D("u_shadowMapDirectLight", dummyTextureUnit);
        }

        /* Point lights */
        glm::mat4 *shadowMVPArray = new glm::mat4[pointLights.size()];
        GLuint texturesArray[OpenGLLightingShader::MAX_LIGHTS];

        for (uint32_t numLight = 0; numLight < pointLights.size(); ++numLight) {
            shader.setPointLight(numLight, *pointLights[numLight]);

            /* Calculate adjusted shadow map matrix */
            glm::mat4 shadowMVP =
                pointLights[numLight]->getProjectionMatrix() * pointLights[numLight]->getViewMatrix() * model3D.getModelMatrix();

            shadowMVPArray[numLight] = biasMatrix * shadowMVP;
            texturesArray[numLight] = textureUnit;

            __(glActiveTexture(GL_TEXTURE0 + textureUnit));
            pointLights[numLight]->getShadowMap()->bindDepth();

            textureUnit++;
        }
        for (uint32_t numLight = pointLights.size(); numLight < OpenGLLightingShader::MAX_LIGHTS; ++numLight) {
            texturesArray[numLight] = dummyTextureUnit;
        }

        shader.setUniformMat4("u_shadowMVPPointLight[0]", shadowMVPArray, pointLights.size());
        shader.setUniformTexture2DArray("u_shadowMapPointLight[0]", texturesArray, OpenGLLightingShader::MAX_LIGHTS);
        shader.setUniformUint("u_numPointLights", pointLights.size());

        /* Free the resources */
        delete[] shadowMVPArray;

        /* Spotlights */
        shadowMVPArray = new glm::mat4[spotLights.size()];

        for (uint32_t numLight = 0; numLight < spotLights.size(); ++numLight) {
            shader.setSpotLight(numLight, *spotLights[numLight]);

            /* Calculate adjusted shadow map matrix */
            glm::mat4 shadowMVP =
                spotLights[numLight]->getProjectionMatrix() * spotLights[numLight]->getViewMatrix() * model3D.getModelMatrix();

            shadowMVPArray[numLight] = biasMatrix * shadowMVP;
            texturesArray[numLight] = textureUnit;

            __(glActiveTexture(GL_TEXTURE0 + textureUnit));
            spotLights[numLight]->getShadowMap()->bindDepth();

            textureUnit++;
        }
        for (uint32_t numLight = spotLights.size(); numLight < OpenGLLightingShader::MAX_LIGHTS; ++numLight) {
            texturesArray[numLight] = dummyTextureUnit;
        }

        shader.setUniformMat4("u_shadowMVPSpotLight[0]", shadowMVPArray, spotLights.size());
        shader.setUniformTexture2DArray("u_shadowMapSpotLight[0]", texturesArray, OpenGLLightingShader::MAX_LIGHTS);
        shader.setUniformUint("u_numSpotLights", spotLights.size());

        /* Free the resources */
        delete[] shadowMVPArray;

        /* Draw the model */
        __(glBindVertexArray(glObject.getVertexArrayID()));
        {
            __(glActiveTexture(GL_TEXTURE0));

            std::vector<Material> materials = glObject.getMaterials();
            std::vector<uint32_t> texturesIDs = glObject.getTexturesIDs();
            std::vector<uint32_t> offset = glObject.getIndicesOffsets();
            std::vector<uint32_t> count = glObject.getIndicesCount();

            for (size_t i = 0; i < materials.size(); ++i) {
                __(glBindTexture(GL_TEXTURE_2D, texturesIDs[i]));
                shader.setMaterial(materials[i]);

                __(glDrawElements(GL_TRIANGLES, count[i], GL_UNSIGNED_INT, (void *)(offset[i] * sizeof(GLuint))));
            }
        }
        __(glBindVertexArray(0));

        /* Set the shader custom parameters */
        shader.setCustomParams();

        /* Unbind */
        shader.detach();
    }
    renderTarget.unbind();

    return true;
}

bool OpenGLRenderer::renderToShadowMap(Model3D &model3D, Light &light, NormalShadowMapShader &shader)
{
    /* Calculate MVP matrix */
    glm::mat4 MVP = light.getProjectionMatrix() * light.getViewMatrix() * model3D.getModelMatrix();

    /* Calculate normal matrix */
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model3D.getModelMatrix())));

    /* Cast the model into an internal type */
    OpenGLModel3D &glObject = dynamic_cast<OpenGLModel3D &>(model3D);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    /* Bind the render target */
    light.getShadowMap()->bind();
    {
        /* Bind program to upload the uniform */
        shader.attach();

        /* Send our transformation to the currently bound shader, in the "MVP" uniform */
        shader.setUniformMat4("u_MVPMatrix", &MVP);

        /* Draw the model */
        __(glBindVertexArray(glObject.getVertexArrayID()));
        {
            std::vector<uint32_t> offset = glObject.getIndicesOffsets();
            std::vector<uint32_t> count = glObject.getIndicesCount();

            for (size_t i = 0; i < count.size(); ++i) {
                __(glDrawElements(GL_TRIANGLES, count[i], GL_UNSIGNED_INT, (void *)(offset[i] * sizeof(GLuint))));
            }
        }
        __(glBindVertexArray(0));

        /* Unbind */
        shader.detach();
    }
    light.getShadowMap()->unbind();

    return true;
}

bool OpenGLRenderer::renderLight(Light &light, Camera &camera, RenderTarget &renderTarget)
{
    glm::vec3 ambient = (light.getAmbient() + light.getDiffuse() + light.getSpecular()) / 3.0f;

    /* TODO: Create this in the renderer so it does not have to be created every time */
    Shader *shader = Shader::New();

    std::string error;
    if (shader->use("utils/render_light", error) != true) {
        log("ERROR loading utils/render_light shader: %s\n", error.c_str());
        return false;
    }

    /* Calculate MVP matrix */
    glm::mat4 MVP = camera.getPerspectiveMatrix() * camera.getViewMatrix() * light.getModelMatrix();

    glEnable(GL_DEPTH_TEST);

    /* Bind the render target */
    renderTarget.bind();
    {
        GLuint lightPosVAO, lightPosVBO;

        /* Bind program to upload the uniform */
        shader->attach();

        /* Send our transformation to the currently bound shader, in the "MVP" uniform */
        shader->setUniformMat4("u_MVPMatrix", &MVP);
        shader->setUniformVec3("u_lightColor", ambient);

        __(glGenVertexArrays(1, &lightPosVAO));
        __(glBindVertexArray(lightPosVAO));

        __(glGenBuffers(1, &lightPosVBO));

        __(glBindBuffer(GL_ARRAY_BUFFER, lightPosVBO));
        __(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), &light.getPosition()[0], GL_STATIC_DRAW));

        __(glEnable(GL_PROGRAM_POINT_SIZE));
        __(glDrawArrays(GL_POINTS, 0, 1));

        __(glDisable(GL_PROGRAM_POINT_SIZE));

        __(glBindVertexArray(0));

        __(glDeleteBuffers(1, &lightPosVBO));
        __(glDeleteVertexArrays(1, &lightPosVAO));

        /* Unbind */
        shader->detach();
    }
    renderTarget.unbind();

    Shader::Delete(shader);

    return true;
}

bool OpenGLRenderer::renderBoundingBox(const BoundingBox &box, const glm::mat4 &modelMatrix, const glm::vec3 &color, Camera &camera,
                                       RenderTarget &renderTarget)
{
    /* TODO: Create this in the renderer so it does not have to be created every time */
    Shader *shader = Shader::New();

    std::string error;
    if (shader->use("utils/render_boundingbox", error) != true) {
        log("ERROR loading utils/render_boundingbox shader: %s\n", error.c_str());
        return false;
    }

    /* Generate the box lines */
    GLfloat boxFaces[] = {
        /* First face */
        box.getMin().x, box.getMin().y, box.getMin().z, box.getMax().x, box.getMin().y, box.getMin().z, box.getMin().x, box.getMin().y,
        box.getMin().z, box.getMin().x, box.getMax().y, box.getMin().z, box.getMin().x, box.getMin().y, box.getMin().z, box.getMin().x,
        box.getMin().y, box.getMax().z,
        /*--*/
        box.getMax().x, box.getMax().y, box.getMax().z, box.getMin().x, box.getMax().y, box.getMax().z, box.getMax().x, box.getMax().y,
        box.getMax().z, box.getMax().x, box.getMin().y, box.getMax().z, box.getMax().x, box.getMax().y, box.getMax().z, box.getMax().x,
        box.getMax().y, box.getMin().z,
        /*--*/
        box.getMin().x, box.getMax().y, box.getMax().z, box.getMin().x, box.getMax().y, box.getMin().z, box.getMin().x, box.getMax().y,
        box.getMax().z, box.getMin().x, box.getMin().y, box.getMax().z,
        /*--*/
        box.getMax().x, box.getMin().y, box.getMax().z, box.getMax().x, box.getMin().y, box.getMin().z, box.getMax().x, box.getMin().y,
        box.getMax().z, box.getMin().x, box.getMin().y, box.getMax().z,
        /*--*/
        box.getMax().x, box.getMax().y, box.getMin().z, box.getMin().x, box.getMax().y, box.getMin().z, box.getMax().x, box.getMax().y,
        box.getMin().z, box.getMax().x, box.getMin().y, box.getMin().z,
    };

    /* Calculate MVP matrix, bounding box coordinates are already in world coordinates */
    glm::mat4 MVP = camera.getPerspectiveMatrix() * camera.getViewMatrix() * modelMatrix;

    __(glEnable(GL_DEPTH_TEST));

    /* Bind the render target */
    renderTarget.bind();
    {
        GLuint boxPosVAO, boxPosVBO;

        /* Bind program to upload the uniform */
        shader->attach();

        /* Send our transformation to the currently bound shader, in the "MVP" uniform */
        shader->setUniformMat4("u_MVPMatrix", &MVP);
        shader->setUniformVec3("u_boxColor", const_cast<glm::vec3 &>(color));

        __(glGenVertexArrays(1, &boxPosVAO));
        __(glBindVertexArray(boxPosVAO));

        __(glGenBuffers(1, &boxPosVBO));

        __(glBindBuffer(GL_ARRAY_BUFFER, boxPosVBO));

        __(glBufferData(GL_ARRAY_BUFFER, sizeof boxFaces, boxFaces, GL_STATIC_DRAW));

        __(glEnableVertexAttribArray(0));
        __(glVertexAttribPointer(0,         // attribute. No particular reason for 0, but must match the layout in the shader.
                                 3,         // size
                                 GL_FLOAT,  // type
                                 GL_FALSE,  // normalized?
                                 0,         // stride
                                 (void *)0  // array buffer offset
                                 ));

        __(glDrawArrays(GL_LINES, 0, sizeof boxFaces / (2 * sizeof *boxFaces)));

        __(glBindVertexArray(0));

        __(glDeleteBuffers(1, &boxPosVBO));
        __(glDeleteVertexArrays(1, &boxPosVAO));

        /* Unbind */
        shader->detach();
    }
    renderTarget.unbind();

    __(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    __(glEnable(GL_CULL_FACE));

    Shader::Delete(shader);

    return true;
}

bool OpenGLRenderer::renderBoundingSphere(const BoundingSphere &sphere, const glm::vec3 &center, const glm::vec3 &color, Camera &camera,
                                          RenderTarget &renderTarget)
{
    /* TODO: Create this in the renderer so it does not have to be created every time */
    Shader *shader = Shader::New();

    std::string error;
    if (shader->use("utils/render_boundingsphere", error) != true) {
        log("ERROR loading utils/render_boundingsphere shader: %s\n", error.c_str());
        return false;
    }

    /* Calculate MVP matrix, bounding box coordinates are already in world coordinates */
    glm::mat4 MVP = camera.getPerspectiveMatrix() * camera.getViewMatrix();

    __(glEnable(GL_DEPTH_TEST));

    /* Bind the render target */
    renderTarget.bind();
    {
        GLuint boxPosVAO, boxPosVBO;

        /* Bind program to upload the uniform */
        shader->attach();

        /* Send our transformation to the currently bound shader, in the "MVP" uniform */
        shader->setUniformMat4("u_MVPMatrix", &MVP);
        shader->setUniformMat4("u_projectionMatrix", &camera.getPerspectiveMatrix());
        shader->setUniformVec3("u_boxColor", const_cast<glm::vec3 &>(color));
        shader->setUniformFloat("u_radius", sphere.getRadius());

        __(glGenVertexArrays(1, &boxPosVAO));
        __(glBindVertexArray(boxPosVAO));

        __(glGenBuffers(1, &boxPosVBO));

        __(glBindBuffer(GL_ARRAY_BUFFER, boxPosVBO));

        __(glBufferData(GL_ARRAY_BUFFER, sizeof center, &center[0], GL_STATIC_DRAW));

        __(glEnableVertexAttribArray(0));
        __(glVertexAttribPointer(0,         // attribute. No particular reason for 0, but must match the layout in the shader.
                                 3,         // size
                                 GL_FLOAT,  // type
                                 GL_FALSE,  // normalized?
                                 0,         // stride
                                 (void *)0  // array buffer offset
                                 ));

        __(glDrawArrays(GL_POINTS, 0, 1));

        __(glBindVertexArray(0));

        __(glDeleteBuffers(1, &boxPosVBO));
        __(glDeleteVertexArrays(1, &boxPosVAO));

        /* Unbind */
        shader->detach();
    }
    renderTarget.unbind();

    __(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    __(glEnable(GL_CULL_FACE));

    Shader::Delete(shader);

    return true;
}
bool OpenGLRenderer::renderModelBoundingBoxes(Model3D &model, Camera &camera, RenderTarget &renderTarget, bool showSphere, bool showAABB,
                                              bool showOOBB)
{
    if (showSphere) {
        if (renderBoundingSphere(model.getBoundingSphere(), model.getPosition(), glm::vec3(1.0f, 0.0f, 0.0f), camera, renderTarget) ==
            false) {
            return false;
        }
    }
    if (showAABB) {
        if (renderBoundingBox(model.getAABB(), glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f), camera, renderTarget) == false) {
            return false;
        }
    }
    if (showOOBB) {
        if (renderBoundingBox(model.getOOBB(), model.getModelMatrix(), glm::vec3(0.0f, 0.0f, 1.0f), camera, renderTarget) == false) {
            return false;
        }
    }
    return true;
}

bool OpenGLRenderer::renderModelNormals(Model3D &model3D, Camera &camera, RenderTarget &renderTarget)
{
    /* TODO: Create this in the renderer so it does not have to be created every time */
    Shader *shader = Shader::New();

    std::string error;
    if (shader->use("utils/render_normals", error) != true) {
        log("ERROR loading utils/render_normals shader: %s\n", error.c_str());
        return false;
    }

    /* Calculate MVP matrix */
    glm::mat4 MVP = camera.getPerspectiveMatrix() * camera.getViewMatrix() * model3D.getModelMatrix();

    /* Calculate normal matrix */
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model3D.getModelMatrix())));

    /* Cast the model into an internal type */
    OpenGLModel3D &glObject = dynamic_cast<OpenGLModel3D &>(model3D);

    /* Bind the render target */
    renderTarget.bind();
    {
        /* Bind program to upload the uniform */
        shader->attach();

        shader->setUniformMat4("u_MVPMatrix", &MVP);

        /* Draw the model */
        __(glBindVertexArray(glObject.getVertexArrayID()));
        {
            std::vector<uint32_t> offset = glObject.getIndicesOffsets();
            std::vector<uint32_t> count = glObject.getIndicesCount();

            for (size_t i = 0; i < offset.size(); ++i) {
                __(glDrawElements(GL_TRIANGLES, count[i], GL_UNSIGNED_INT, (void *)(offset[i] * sizeof(GLuint))));
            }
        }
        __(glBindVertexArray(0));

        /* Unbind */
        shader->detach();
    }
    renderTarget.unbind();

    Shader::Delete(shader);

    return true;
}

bool OpenGLRenderer::resize(uint16_t width, uint16_t height)
{
    _width = width;
    _height = height;
    return true;
}

void OpenGLRenderer::flush() { glFinish(); }
void OpenGLRenderer::clear()
{
    __(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    __(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    __(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}
