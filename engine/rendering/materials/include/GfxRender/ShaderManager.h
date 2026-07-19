#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace RBX
{
namespace Graphics
{

class VertexShader;
class FragmentShader;
class ShaderProgram;
class VisualEngine;

class ShaderManager
{
public:
    ShaderManager(VisualEngine* visualEngine);
    ~ShaderManager();
    
    void loadShaders(const std::string& folder, const std::string& language, bool consoleOutput);

    std::shared_ptr<ShaderProgram> getProgram(const std::string& vsName, const std::string& fsName);

    std::shared_ptr<ShaderProgram> getProgramOrFFP(const std::string& vsName, const std::string& fsName);
    std::shared_ptr<ShaderProgram> getProgramFFP();

private:
    VisualEngine* visualEngine;

    using VertexShaders = std::unordered_map<std::string, std::shared_ptr<VertexShader>>;
    VertexShaders vertexShaders;

    using FragmentShaders = std::unordered_map<std::string, std::shared_ptr<FragmentShader>>;
    FragmentShaders fragmentShaders;

    using ShaderPrograms = std::unordered_map<std::string, std::shared_ptr<ShaderProgram>>;
    ShaderPrograms shaderPrograms;

    std::shared_ptr<ShaderProgram> shaderProgramFFP;

    std::shared_ptr<ShaderProgram> createProgram(const std::string& name, const std::string& vsName, const std::string& fsName);
};

}
}
