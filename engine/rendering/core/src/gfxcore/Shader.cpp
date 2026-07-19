#include "GfxCore/Shader.h"

#include <sstream>
#include <iostream>

namespace RBX
{
namespace Graphics
{

VertexShader::VertexShader(Device* device)
    : Resource(device)
{
}

VertexShader::~VertexShader()
{
}

FragmentShader::FragmentShader(Device* device)
    : Resource(device)
{
}

FragmentShader::~FragmentShader()
{
}

ShaderProgram::ShaderProgram(Device* device, const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader)
	: Resource(device)
    , vertexShader(vertexShader)
    , fragmentShader(fragmentShader)
{
}

ShaderProgram::~ShaderProgram()
{
}

void ShaderProgram::dumpToFLog(const std::string& text, int channel)
{
    std::vector<std::string> messages;
    std::istringstream stream(text);
    for (std::string line; std::getline(stream, line);)
        messages.push_back(line);

	while (!messages.empty() && messages.back().empty())
		messages.pop_back();

    for (size_t i = 0; i < messages.size(); ++i)
        std::clog << "shader[" << channel << "]: " << messages[i] << '\n';
}

ShaderGlobalConstant::ShaderGlobalConstant(const char* name, unsigned int offset, unsigned int size)
	: name(name)
    , offset(offset)
    , size(size)
{
}

}
}
