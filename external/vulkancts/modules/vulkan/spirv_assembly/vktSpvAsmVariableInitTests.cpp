/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief SPIR-V Assembly Tests for OpVariable initializer
 *//*--------------------------------------------------------------------*/

#include "vktSpvAsmVariableInitTests.hpp"
#include "vktSpvAsmComputeShaderCase.hpp"
#include "vktSpvAsmComputeShaderTestUtil.hpp"
#include "vktSpvAsmGraphicsShaderTestUtil.hpp"
#include "tcuStringTemplate.hpp"

namespace vkt
{
namespace SpirVAssembly
{

using namespace vk;
using std::map;
using std::string;
using std::vector;
using tcu::IVec3;
using tcu::Vec4;
using tcu::RGBA;
using tcu::StringTemplate;

namespace
{

void addShaderCodeOutputFloat		(vk::SourceCollections& dst, InstanceContext context);
void addShaderCodeOutputVector		(vk::SourceCollections& dst, InstanceContext context);
void addShaderCodeOutputMatrix		(vk::SourceCollections& dst, InstanceContext context);
void addShaderCodeOutputFloatArray	(vk::SourceCollections& dst, InstanceContext context);
void addShaderCodeOutputStruct		(vk::SourceCollections& dst, InstanceContext context);

struct TestParams
{
	string											name;
	string											type;
	int												numComponents;
	FunctionPrograms1<InstanceContext>::Function	shaderInit;
};

const TestParams	params[]	=
{
	{ "float",		"f32",			1				, addShaderCodeOutputFloat		},
	{ "vec4",		"v4f32",		4				, addShaderCodeOutputVector		},
	{ "matrix",		"matrix",		2 * 4			, addShaderCodeOutputMatrix		},
	{ "floatarray",	"floatArray",	8				, addShaderCodeOutputFloatArray	},
	{ "struct",		"struct",		2 * 4 + 4 + 4	, addShaderCodeOutputStruct		}
};

const string		common		=
	"                %f32_1 = OpConstant %f32 1\n"
	"              %v4f32_1 = OpConstantComposite %v4f32 %f32_1 %f32_1 %f32_1 %f32_1\n"
	"               %matrix = OpTypeMatrix %v4f32 2\n"
	"             %matrix_1 = OpConstantComposite %matrix %v4f32_1 %v4f32_1\n"
	"               %struct = OpTypeStruct %matrix %v4f32 %f32 %f32 %f32 %f32\n"
	"             %struct_1 = OpConstantComposite %struct %matrix_1 %v4f32_1 %f32_1 %f32_1 %f32_1 %f32_1\n"
	"              %c_u32_8 = OpConstant %u32 8\n"
	"           %floatArray = OpTypeArray %f32 %c_u32_8\n"
	"         %floatArray_1 = OpConstantComposite %floatArray %f32_1 %f32_1 %f32_1 %f32_1 %f32_1 %f32_1 %f32_1 %f32_1\n"
	"          %numElements = OpConstant %u32 ${count}\n"
	"          %outputArray = OpTypeArray %${type} %numElements\n"
	"               %Output = OpTypeStruct %outputArray\n"
	"          %_ptr_Output = OpTypePointer StorageBuffer %Output\n"
	"                %sbPtr = OpTypePointer StorageBuffer %${type}\n"
	"           %dataOutput = OpVariable %_ptr_Output StorageBuffer\n";

const string		decorations	=
	"                         OpDecorate %outputArray ArrayStride ${arrayStride}\n"
	"                         OpMemberDecorate %Output 0 Offset 0\n"
	"                         OpDecorate %Output Block\n"
	"                         OpDecorate %dataOutput DescriptorSet 0\n"
	"                         OpDecorate %dataOutput Binding 0\n"
	"                         OpDecorate %floatArray ArrayStride 4\n"
	"                         OpMemberDecorate %struct 0 ColMajor\n"
	"                         OpMemberDecorate %struct 0 Offset 0\n"
	"                         OpMemberDecorate %struct 0 MatrixStride 16\n"
	"                         OpMemberDecorate %struct 1 Offset 32\n"
	"                         OpMemberDecorate %struct 2 Offset 48\n"
	"                         OpMemberDecorate %struct 3 Offset 52\n"
	"                         OpMemberDecorate %struct 4 Offset 56\n"
	"                         OpMemberDecorate %struct 5 Offset 60\n"
	"${extraDecorations:opt}";

void addComputeVariableInitPrivateTest (tcu::TestCaseGroup* group)
{
	tcu::TestContext&	testCtx			= group->getTestContext();
	const int			numFloats		= 128;
	tcu::TestCaseGroup*	privateGroup	= new tcu::TestCaseGroup(testCtx, "private", "Tests OpVariable initialization in private storage class.");
	ComputeShaderSpec	spec;
	vector<float>		expectedOutput;

	const StringTemplate shaderSourceTemplate (
		string(
		"                         OpCapability Shader\n"
		"                         OpExtension \"SPV_KHR_storage_buffer_storage_class\"\n"
		"                    %1 = OpExtInstImport \"GLSL.std.450\"\n"
		"                         OpMemoryModel Logical GLSL450\n"
		"                         OpEntryPoint GLCompute %main \"main\" %gl_GlobalInvocationID\n"
		"                         OpExecutionMode %main LocalSize 1 1 1\n"
		"                         OpSource GLSL 430\n"
		"                         OpDecorate %gl_GlobalInvocationID BuiltIn GlobalInvocationId\n")
		+ decorations + string(
		"                 %void = OpTypeVoid\n"
		"             %voidFunc = OpTypeFunction %void\n"
		"                  %f32 = OpTypeFloat 32\n"
		"                  %u32 = OpTypeInt 32 0\n"
		"              %c_u32_0 = OpConstant %u32 0\n"
		"                %v4f32 = OpTypeVector %f32 4\n")
		+ common + string(
		"              %dataPtr = OpTypePointer Private %${type}\n"
		"   %_ptr_Function_uint = OpTypePointer Function %u32\n"
		"               %v3uint = OpTypeVector %u32 3\n"
		"    %_ptr_Input_v3uint = OpTypePointer Input %v3uint\n"
		"%gl_GlobalInvocationID = OpVariable %_ptr_Input_v3uint Input\n"
		"      %_ptr_Input_uint = OpTypePointer Input %u32\n"
		"                  %int = OpTypeInt 32 1\n"
		"                %int_0 = OpConstant %int 0\n"
		"                   %f1 = OpVariable %dataPtr Private %${constData}\n"
		"                 %main = OpFunction %void None %voidFunc\n"
		"                %entry = OpLabel\n"
		"        %invocationPtr = OpAccessChain %_ptr_Input_uint %gl_GlobalInvocationID %c_u32_0\n"
		"           %invocation = OpLoad %u32 %invocationPtr\n"
		"           %outputData = OpLoad %${type} %f1\n"
		"            %outputPtr = OpAccessChain %sbPtr %dataOutput %int_0 %invocation\n"
		"                         OpStore %outputPtr %outputData\n"
		"                         OpReturn\n"
		"                         OpFunctionEnd\n"));

	group->addChild(privateGroup);

	expectedOutput.reserve(numFloats);
	for (deUint32 numIdx = 0; numIdx < numFloats; ++numIdx)
		expectedOutput.push_back(1.0f);

	spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));

	for (int paramIdx = 0; paramIdx < DE_LENGTH_OF_ARRAY(params); paramIdx++)
	{
		map<string, string> shaderSpec;
		const int			numComponents	= params[paramIdx].numComponents;
		const int			numElements		= numFloats / numComponents;

		shaderSpec["type"]			= params[paramIdx].type;
		shaderSpec["arrayStride"]	= de::toString(numComponents * 4);
		shaderSpec["count"]			= de::toString(numElements);
		shaderSpec["constData"]		= params[paramIdx].type + "_1";

		if (params[paramIdx].type == "matrix")
		{
			shaderSpec["extraDecorations"] =
				"                         OpMemberDecorate %Output 0 ColMajor\n"
				"                         OpMemberDecorate %Output 0 MatrixStride 16\n";
		}

		spec.assembly				= shaderSourceTemplate.specialize(shaderSpec);
		spec.numWorkGroups			= IVec3(numElements, 1, 1);
		spec.extensions.push_back("VK_KHR_storage_buffer_storage_class");

		privateGroup->addChild(new SpvAsmComputeShaderCase(testCtx, params[paramIdx].name.c_str(), "", spec));
	}
}

void addGraphicsVariableInitPrivateTest (tcu::TestCaseGroup* group)
{
	tcu::TestContext&		testCtx				= group->getTestContext();
	map<string, string>		fragments;
	RGBA					defaultColors[4];
	GraphicsResources		resources;
	vector<string>			extensions;
	tcu::TestCaseGroup*		privateGroup		= new tcu::TestCaseGroup(testCtx, "private", "Tests OpVariable initialization in private storage class.");
	const int				numFloats			= 128;
	vector<float>			expectedOutput;

	StringTemplate			preMain				(
		common +
		"              %dataPtr = OpTypePointer Private %${type}\n"
		"                   %f1 = OpVariable %dataPtr Private %${constData}\n"
		);

	StringTemplate			decoration			(decorations);

	StringTemplate			testFun				(
		"            %test_code = OpFunction %v4f32 None %v4f32_function\n"
		"                %param = OpFunctionParameter %v4f32\n"
		"                %entry = OpLabel\n"
		"           %outputData = OpLoad %${type} %f1\n"
		"                    %i = OpVariable %fp_i32 Function\n"
		"                         OpStore %i %c_i32_0\n"
		"                         OpBranch %loop\n"
		"                 %loop = OpLabel\n"
		"                   %15 = OpLoad %i32 %i\n"
		"                   %lt = OpSLessThan %bool %15 %numElements\n"
		"                         OpLoopMerge %merge %inc None\n"
		"                         OpBranchConditional %lt %write %merge\n"
		"                %write = OpLabel\n"
		"                   %30 = OpLoad %i32 %i\n"
		"            %outputPtr = OpAccessChain %sbPtr %dataOutput %c_i32_0 %30\n"
		"                         OpStore %outputPtr %outputData\n"
		"                         OpBranch %inc\n"
		"                  %inc = OpLabel\n"
		"                   %37 = OpLoad %i32 %i\n"
		"                   %39 = OpIAdd %i32 %37 %c_i32_1\n"
		"                         OpStore %i %39\n"
		"                         OpBranch %loop\n"
		"                %merge = OpLabel\n"
		"                         OpReturnValue %param\n"
		"                         OpFunctionEnd\n");


	group->addChild(privateGroup);

	getDefaultColors(defaultColors);

	expectedOutput.reserve(numFloats);
	for (deUint32 numIdx = 0; numIdx < numFloats; ++numIdx)
		expectedOutput.push_back(1.0f);

	resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(expectedOutput))));
	extensions.push_back("VK_KHR_storage_buffer_storage_class");

	for (int paramIdx = 0; paramIdx < DE_LENGTH_OF_ARRAY(params); paramIdx++)
	{
		map<string, string> shaderSpec;
		const int			numComponents	= params[paramIdx].numComponents;
		const int			numElements		= numFloats / numComponents;

		shaderSpec["type"]			= params[paramIdx].type;
		shaderSpec["arrayStride"]	= de::toString(numComponents * 4);
		shaderSpec["count"]			= de::toString(numElements);
		shaderSpec["constData"]		= params[paramIdx].type + "_1";

		if (params[paramIdx].type == "matrix")
		{
			shaderSpec["extraDecorations"] =
				"                         OpMemberDecorate %Output 0 ColMajor\n"
				"                         OpMemberDecorate %Output 0 MatrixStride 16\n";
		}

		fragments["extension"]		= "OpExtension \"SPV_KHR_storage_buffer_storage_class\"\n";
		fragments["pre_main"]		= preMain.specialize(shaderSpec);
		fragments["decoration"]		= decoration.specialize(shaderSpec);
		fragments["testfun"]		= testFun.specialize(shaderSpec);

		createTestsForAllStages(params[paramIdx].name, defaultColors, defaultColors, fragments, resources, extensions, privateGroup);
	}
}

void addShaderCodeOutput(vk::SourceCollections& dst, InstanceContext& context, string type)
{
	SpirvVersion			targetSpirvVersion	= context.resources.spirvVersion;
	map<string, string>		spec;

	// Needed for preventing duplicate pointer declarations.
	if (type == "v4f32")
	{
		spec["vec4ptrDeclOutput"]	= "";
		spec["vec4ptrOutput"]		= "outputPtr";
		spec["vec4ptrDeclInput"]	= "";
		spec["vec4ptrInput"]		= "inputPtr";
	}
	else
	{
		spec["vec4ptrDeclOutput"]	= "     %_ptr_Output_v4f32 = OpTypePointer Output %v4f32\n";
		spec["vec4ptrOutput"]		= "_ptr_Output_v4f32";
		spec["vec4ptrDeclInput"]	= "     %_ptr_Input_v4f32 = OpTypePointer Input %v4f32\n";
		spec["vec4ptrInput"]		= "_ptr_Input_v4f32";
	}

	const string			types				=
		"                     %u32 = OpTypeInt 32 0\n"
		"                     %f32 = OpTypeFloat 32\n"
		"                   %v4f32 = OpTypeVector %f32 4\n"
		"                  %matrix = OpTypeMatrix %v4f32 2\n"
		"                 %c_u32_0 = OpConstant %u32 0\n"
		"                 %c_u32_8 = OpConstant %u32 8\n"
		"              %floatArray = OpTypeArray %f32 %c_u32_8\n";

	string					outputDecoration	= "                       OpDecorate %Output Block\n";

	if (type == "matrix")
	{
		spec["extraDecorations"] =
			"                       OpMemberDecorate %Output 0 ColMajor\n"
			"                       OpMemberDecorate %Output 0 MatrixStride 16\n";
	}

	// Output selected data type with all components having value one.
	const StringTemplate	vertexShader		(
		string(
		"                            OpCapability Shader\n"
		"                       %1 = OpExtInstImport \"GLSL.std.450\"\n"
		"                            OpMemoryModel Logical GLSL450\n"
		"                            OpEntryPoint Vertex %main \"main\" %_ %position %vtxColor %color %outData\n"
		"                            OpSource GLSL 430\n"
		"                            OpMemberDecorate %gl_PerVertex 0 BuiltIn Position\n"
		"                            OpMemberDecorate %gl_PerVertex 1 BuiltIn PointSize\n"
		"                            OpMemberDecorate %gl_PerVertex 2 BuiltIn ClipDistance\n"
		"                            OpDecorate %gl_PerVertex Block\n"
		"                            OpDecorate %position Location 0\n"
		"                            OpDecorate %vtxColor Location 1\n"
		"                            OpDecorate %color Location 1\n"
		"                            OpDecorate %outData Location 2\n"
		"                            OpDecorate %floatArray ArrayStride 4\n"
		"                    %void = OpTypeVoid\n"
		"                       %3 = OpTypeFunction %void\n")
		+ types + string(
		"                   %f32_1 = OpConstant %f32 1\n"
		"                 %v4f32_1 = OpConstantComposite %v4f32 %f32_1 %f32_1 %f32_1 %f32_1\n"
		"                %matrix_1 = OpConstantComposite %matrix %v4f32_1 %v4f32_1\n"
		"            %floatArray_1 = OpConstantComposite %floatArray %f32_1 %f32_1 %f32_1 %f32_1 %f32_1 %f32_1 %f32_1 %f32_1\n"
		"                 %c_u32_1 = OpConstant %u32 1\n"
		"          %_arr_f32_u32_1 = OpTypeArray %f32 %c_u32_1\n"
		"            %gl_PerVertex = OpTypeStruct %v4f32 %f32 %_arr_f32_u32_1\n"
		"%_ptr_Output_gl_PerVertex = OpTypePointer Output %gl_PerVertex\n"
		"                       %_ = OpVariable %_ptr_Output_gl_PerVertex Output\n"
		"               %outputPtr = OpTypePointer Output %${type}\n"
		"                 %outData = OpVariable %outputPtr Output %${type}_1\n"
		"        %_ptr_Input_v4f32 = OpTypePointer Input %v4f32\n"
		"                %position = OpVariable %_ptr_Input_v4f32 Input\n"
		"${vec4ptrDeclOutput}"
		"                %vtxColor = OpVariable %${vec4ptrOutput} Output\n"
		"                   %color = OpVariable %_ptr_Input_v4f32 Input\n"
		"                    %main = OpFunction %void None %3\n"
		"                   %entry = OpLabel\n"
		"                 %posData = OpLoad %v4f32 %position\n"
		"            %posOutputPtr = OpAccessChain %${vec4ptrOutput} %_ %c_u32_0\n"
		"                            OpStore %posOutputPtr %posData\n"
		"               %colorData = OpLoad %v4f32 %color\n"
		"                            OpStore %vtxColor %colorData\n"
		"                            OpReturn\n"
		"                            OpFunctionEnd\n"));

	// Pass incoming data into buffer
	const StringTemplate	fragmentShader		(
		string(
		"                       OpCapability Shader\n"
		"                       OpExtension \"SPV_KHR_storage_buffer_storage_class\"\n"
		"                  %1 = OpExtInstImport \"GLSL.std.450\"\n"
		"                       OpMemoryModel Logical GLSL450\n"
		"                       OpEntryPoint Fragment %main \"main\" %fragColor %vtxColor %inData\n"
		"                       OpExecutionMode %main OriginUpperLeft\n"
		"                       OpSource GLSL 430\n"
		"                       OpDecorate %fragColor Location 0\n"
		"                       OpDecorate %vtxColor Location 1\n"
		"                       OpMemberDecorate %Output 0 Offset 0\n"
		"                       OpDecorate %Output Block\n"
		"                       OpDecorate %dataOutput DescriptorSet 0\n"
		"                       OpDecorate %dataOutput Binding 0\n"
		"                       OpDecorate %inData Location 2\n"
		"                       OpDecorate %floatArray ArrayStride 4\n"
		"${extraDecorations:opt}"
		"               %void = OpTypeVoid\n"
		"                  %3 = OpTypeFunction %void\n")
		+ types + string(
		"           %inputPtr = OpTypePointer Input %${type}\n"
		"             %inData = OpVariable %inputPtr Input\n"
		"  %_ptr_Output_v4f32 = OpTypePointer Output %v4f32\n"
		"          %fragColor = OpVariable %_ptr_Output_v4f32 Output\n"
		"${vec4ptrDeclInput}"
		"           %vtxColor = OpVariable %${vec4ptrInput} Input\n"
		"             %Output = OpTypeStruct %${type}\n"
		"        %_ptr_Output = OpTypePointer StorageBuffer %Output\n"
		"         %dataOutput = OpVariable %_ptr_Output StorageBuffer\n"
		"          %outputPtr = OpTypePointer StorageBuffer %${type}\n"
		"               %main = OpFunction %void None %3\n"
		"              %entry = OpLabel\n"
		"          %colorData = OpLoad %v4f32 %vtxColor\n"
		"                       OpStore %fragColor %colorData\n"
		"          %inputData = OpLoad %${type} %inData\n"
		"      %outputDataPtr = OpAccessChain %outputPtr %dataOutput %c_u32_0\n"
		"                       OpStore %outputDataPtr %inputData\n"
		"                       OpReturn\n"
		"                       OpFunctionEnd\n"));

	spec["type"] = type;

	dst.spirvAsmSources.add("vert", DE_NULL) << vertexShader.specialize(spec) << SpirVAsmBuildOptions(targetSpirvVersion);
	dst.spirvAsmSources.add("frag", DE_NULL) << fragmentShader.specialize(spec) << SpirVAsmBuildOptions(targetSpirvVersion);
}

void addShaderCodeOutputFloat (vk::SourceCollections& dst, InstanceContext context)
{
	addShaderCodeOutput(dst, context, "f32");
}

void addShaderCodeOutputVector (vk::SourceCollections& dst, InstanceContext context)
{
	addShaderCodeOutput(dst, context, "v4f32");
}

void addShaderCodeOutputMatrix (vk::SourceCollections& dst, InstanceContext context)
{
	addShaderCodeOutput(dst, context, "matrix");
}

void addShaderCodeOutputFloatArray (vk::SourceCollections& dst, InstanceContext context)
{
	addShaderCodeOutput(dst, context, "floatArray");
}

void addShaderCodeOutputStruct (vk::SourceCollections& dst, InstanceContext context)
{
	SpirvVersion		targetSpirvVersion	= context.resources.spirvVersion;

	// Output structure of matrix, vec4, and four floats all having values of 1.
	const string	vertexShader	=
		"                            OpCapability Shader\n"
		"                       %1 = OpExtInstImport \"GLSL.std.450\"\n"
		"                            OpMemoryModel Logical GLSL450\n"
		"                            OpEntryPoint Vertex %main \"main\" %_ %position %vtxColor %color %outData\n"
		"                            OpSource GLSL 430\n"
		"                            OpMemberDecorate %gl_PerVertex 0 BuiltIn Position\n"
		"                            OpMemberDecorate %gl_PerVertex 1 BuiltIn PointSize\n"
		"                            OpMemberDecorate %gl_PerVertex 2 BuiltIn ClipDistance\n"
		"                            OpDecorate %gl_PerVertex Block\n"
		"                            OpDecorate %position Location 0\n"
		"                            OpDecorate %vtxColor Location 1\n"
		"                            OpDecorate %color Location 1\n"
		"                            OpDecorate %outData Location 2\n"
		"                            OpMemberDecorate %Data 0 ColMajor\n"
		"                            OpMemberDecorate %Data 0 Offset 0\n"
		"                            OpMemberDecorate %Data 0 MatrixStride 16\n"
		"                            OpMemberDecorate %Data 1 Offset 32\n"
		"                            OpMemberDecorate %Data 2 Offset 48\n"
		"                            OpMemberDecorate %Data 3 Offset 52\n"
		"                            OpMemberDecorate %Data 4 Offset 56\n"
		"                            OpMemberDecorate %Data 5 Offset 60\n"
		"                            OpMemberDecorate %DataOutput 0 Offset 0\n"
		"                    %void = OpTypeVoid\n"
		"                %voidFunc = OpTypeFunction %void\n"
		"                   %float = OpTypeFloat 32\n"
		"                 %v4float = OpTypeVector %float 4\n"
		"                    %uint = OpTypeInt 32 0\n"
		"                  %uint_1 = OpConstant %uint 1\n"
		"       %_arr_float_uint_1 = OpTypeArray %float %uint_1\n"
		"            %gl_PerVertex = OpTypeStruct %v4float %float %_arr_float_uint_1\n"
		"%_ptr_Output_gl_PerVertex = OpTypePointer Output %gl_PerVertex\n"
		"                       %_ = OpVariable %_ptr_Output_gl_PerVertex Output\n"
		"                     %int = OpTypeInt 32 1\n"
		"                   %int_0 = OpConstant %int 0\n"
		"      %_ptr_Input_v4float = OpTypePointer Input %v4float\n"
		"                %position = OpVariable %_ptr_Input_v4float Input\n"
		"     %_ptr_Output_v4float = OpTypePointer Output %v4float\n"
		"                %vtxColor = OpVariable %_ptr_Output_v4float Output\n"
		"                   %color = OpVariable %_ptr_Input_v4float Input\n"
		"             %mat2v4float = OpTypeMatrix %v4float 2\n"
		"                    %Data = OpTypeStruct %mat2v4float %v4float %float %float %float %float\n"
		"              %DataOutput = OpTypeStruct %Data\n"
		"  %_ptr_Output_DataOutput = OpTypePointer Output %DataOutput\n"
		"                 %float_1 = OpConstant %float 1\n"
		"                  %vec4_1 = OpConstantComposite %v4float %float_1 %float_1 %float_1 %float_1\n"
		"                %matrix_1 = OpConstantComposite %mat2v4float %vec4_1 %vec4_1\n"
		" %_ptr_Output_mat2v4float = OpTypePointer Output %mat2v4float\n"
		"       %_ptr_Output_float = OpTypePointer Output %float\n"
		"                  %data_1 = OpConstantComposite %Data %matrix_1 %vec4_1 %float_1 %float_1 %float_1 %float_1\n"
		"            %dataOutput_1 = OpConstantComposite %DataOutput %data_1\n"
		"                 %outData = OpVariable %_ptr_Output_DataOutput Output %dataOutput_1\n"
		"                    %main = OpFunction %void None %voidFunc\n"
		"                   %entry = OpLabel\n"
		"                 %posData = OpLoad %v4float %position\n"
		"                  %posPtr = OpAccessChain %_ptr_Output_v4float %_ %int_0\n"
		"                            OpStore %posPtr %posData\n"
		"               %colorData = OpLoad %v4float %color\n"
		"                            OpStore %vtxColor %colorData\n"
		"                            OpReturn\n"
		"                            OpFunctionEnd\n";

	// Pass the incoming input struct into buffer.
	const string	fragmentShader	=
		"                            OpCapability Shader\n"
		"                            OpExtension \"SPV_KHR_storage_buffer_storage_class\"\n"
		"                       %1 = OpExtInstImport \"GLSL.std.450\"\n"
		"                            OpMemoryModel Logical GLSL450\n"
		"                            OpEntryPoint Fragment %main \"main\" %fragColor %vtxColor %inData\n"
		"                            OpExecutionMode %main OriginUpperLeft\n"
		"                            OpSource GLSL 430\n"
		"                            OpDecorate %fragColor Location 0\n"
		"                            OpDecorate %vtxColor Location 1\n"
		"                            OpMemberDecorate %Data 0 ColMajor\n"
		"                            OpMemberDecorate %Data 0 Offset 0\n"
		"                            OpMemberDecorate %Data 0 MatrixStride 16\n"
		"                            OpMemberDecorate %Data 1 Offset 32\n"
		"                            OpMemberDecorate %Data 2 Offset 48\n"
		"                            OpMemberDecorate %Data 3 Offset 52\n"
		"                            OpMemberDecorate %Data 4 Offset 56\n"
		"                            OpMemberDecorate %Data 5 Offset 60\n"
		"                            OpMemberDecorate %Output 0 Offset 0\n"
		"                            OpDecorate %Output Block\n"
		"                            OpDecorate %dataOutput DescriptorSet 0\n"
		"                            OpDecorate %dataOutput Binding 0\n"
		"                            OpDecorate %inData Location 2\n"
		"                    %void = OpTypeVoid\n"
		"                %voidFunc = OpTypeFunction %void\n"
		"                   %float = OpTypeFloat 32\n"
		"                 %v4float = OpTypeVector %float 4\n"
		"     %_ptr_Output_v4float = OpTypePointer Output %v4float\n"
		"               %fragColor = OpVariable %_ptr_Output_v4float Output\n"
		"      %_ptr_Input_v4float = OpTypePointer Input %v4float\n"
		"                %vtxColor = OpVariable %_ptr_Input_v4float Input\n"
		"             %mat2v4float = OpTypeMatrix %v4float 2\n"
		"                    %Data = OpTypeStruct %mat2v4float %v4float %float %float %float %float\n"
		"                  %Output = OpTypeStruct %Data\n"
		"             %_ptr_Output = OpTypePointer StorageBuffer %Output\n"
		"              %dataOutput = OpVariable %_ptr_Output StorageBuffer\n"
		"                     %int = OpTypeInt 32 1\n"
		"                   %int_0 = OpConstant %int 0\n"
		"               %DataInput = OpTypeStruct %Data\n"
		"    %_ptr_Input_DataInput = OpTypePointer Input %DataInput\n"
		"                  %inData = OpVariable %_ptr_Input_DataInput Input\n"
		"         %_ptr_Input_Data = OpTypePointer Input %Data\n"
		"               %_ptr_Data = OpTypePointer StorageBuffer %Data\n"
		"                    %main = OpFunction %void None %voidFunc\n"
		"                   %entry = OpLabel\n"
		"               %colorData = OpLoad %v4float %vtxColor\n"
		"                            OpStore %fragColor %colorData\n"
		"            %inputDataPtr = OpAccessChain %_ptr_Input_Data %inData %int_0\n"
		"               %inputData = OpLoad %Data %inputDataPtr\n"
		"           %outputDataPtr = OpAccessChain %_ptr_Data %dataOutput %int_0\n"
		"                            OpStore %outputDataPtr %inputData\n"
		"                            OpReturn\n"
		"                            OpFunctionEnd\n";

	dst.spirvAsmSources.add("vert", DE_NULL) << vertexShader << SpirVAsmBuildOptions(targetSpirvVersion);
	dst.spirvAsmSources.add("frag", DE_NULL) << fragmentShader << SpirVAsmBuildOptions(targetSpirvVersion);
}

void addGraphicsVariableInitOutputTest (tcu::TestCaseGroup* group)
{
	tcu::TestContext&		testCtx				= group->getTestContext();
	map<string, string>		fragments;
	RGBA					defaultColors[4];
	tcu::TestCaseGroup*		outputGroup			= new tcu::TestCaseGroup(testCtx, "output", "Tests OpVariable initialization in output storage class.");
	vector<deInt32>			noSpecConstants;
	PushConstants			noPushConstants;
	GraphicsInterfaces		noInterfaces;
	vector<string>			extensions;
	vector<string>			features;
	map<string, string>		noFragments;
	StageToSpecConstantMap	specConstantMap;

	const ShaderElement		pipelineStages[]	=
	{
		ShaderElement("vert", "main", VK_SHADER_STAGE_VERTEX_BIT),
		ShaderElement("frag", "main", VK_SHADER_STAGE_FRAGMENT_BIT),
	};

	specConstantMap[VK_SHADER_STAGE_VERTEX_BIT]		= noSpecConstants;
	specConstantMap[VK_SHADER_STAGE_FRAGMENT_BIT]	= noSpecConstants;

	getDefaultColors(defaultColors);

	group->addChild(outputGroup);

	features.push_back("fragmentStoresAndAtomics");
	extensions.push_back("VK_KHR_storage_buffer_storage_class");

	for (int paramIdx = 0; paramIdx < DE_LENGTH_OF_ARRAY(params); paramIdx++)
	{
		const deUint32		numComponents	= params[paramIdx].numComponents;
		GraphicsResources	resources;
		vector<float>		expectedOutput;

		expectedOutput.reserve(numComponents);
		for (deUint32 numIdx = 0; numIdx < numComponents; ++numIdx)
			expectedOutput.push_back(1.0f);

		resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(expectedOutput))));

		{
			const InstanceContext& instanceContext = createInstanceContext(pipelineStages,
																		   defaultColors,
																		   defaultColors,
																		   noFragments,
																		   specConstantMap,
																		   noPushConstants,
																		   resources,
																		   noInterfaces,
																		   extensions,
																		   features,
																		   VulkanFeatures(),
																		   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
																		   QP_TEST_RESULT_FAIL,
																		   string());


			addFunctionCaseWithPrograms<InstanceContext>(outputGroup,
														 params[paramIdx].name.c_str(),
														 "",
														 params[paramIdx].shaderInit,
														 runAndVerifyDefaultPipeline,
														 instanceContext);
		}
	}
}

} // anonymous

tcu::TestCaseGroup* createVariableInitComputeGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group(new tcu::TestCaseGroup(testCtx, "variable_init", "Compute tests for OpVariable initialization."));
	addComputeVariableInitPrivateTest(group.get());

	return group.release();
}

tcu::TestCaseGroup* createVariableInitGraphicsGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group(new tcu::TestCaseGroup(testCtx, "variable_init", "Graphics tests for OpVariable initialization."));
	addGraphicsVariableInitPrivateTest(group.get());
	addGraphicsVariableInitOutputTest(group.get());

	return group.release();
}

} // SpirVAssembly
} // vkt
