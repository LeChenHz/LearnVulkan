import os
import platform
import shutil
import tempfile

script_path = os.path.dirname(os.path.abspath(__file__))

if platform.system() == "Linux":
    platform_name = "linux-x86_64"
elif platform.system() == "Windows":
    platform_name = "windows-x86_64"
elif platform.system() == "Darwin":
    platform_name = "darwin-x86_64"

glslangValidator_path = os.path.join(script_path, "D:/WorkSpace/nemu-vulkan/nemu-graphics/nemu-renderer/thirdparty/VulkanSDK/1.2.170.0/Bin/glslangValidator.exe")

shader_source_path = script_path
shader_lib_name = "Etc2ShaderLib"
#shader_source_names = {"Etc2RGB8", "Etc2RGBA8", "EacR11Unorm", "EacR11Snorm", "EacRG11Unorm", "EacRG11Snorm", "Astc"}
#image_types = {"1DArray", "2DArray", "3D"}

shader_source_names = {"Astc"}
image_types = {"2DArray"}

unquant_table_string = """
uint kUnquantTritWeightMapBitIdx[4] = {
    0, 3, 9, 21,
};

uint kUnquantTritWeightMap[45] = {
    0, 32, 63,
    0, 63, 12, 51, 25, 38,
    0, 63, 17, 46, 5, 58, 23, 40, 11, 52, 28, 35,
    0, 63, 8, 55, 16, 47, 24, 39, 2, 61, 11, 52, 19, 44, 27, 36, 5, 58, 13, 50, 22, 41, 30, 33,
};

uint kUnquantQuintWeightMapBitIdx[3] = {
    0, 5, 15,
};

uint kUnquantQuintWeightMap[35] = {
    0, 16, 32, 47, 63,
    0, 63, 7, 56, 14, 49, 21, 42, 28, 35,
    0, 63, 16, 47, 3, 60, 19, 44, 6, 57, 23, 40, 9, 54, 26, 37, 13, 50, 29, 34,
};

uint kUnquantTritColorMapBitIdx[7] = {
    0, 3, 9, 21, 45, 93, 189,
};

uint kUnquantTritColorMap[381] = {
    0, 0, 0,
    0, 255, 51, 204, 102, 153,
    0, 255, 69, 186, 23, 232, 92, 163, 46, 209, 116, 139,
    0, 255, 33, 222, 66, 189, 99, 156, 11, 244, 44, 211, 77, 178, 110, 145, 22, 233, 55, 200, 88, 167, 121, 134,
    0, 255, 16, 239, 32, 223, 48, 207, 65, 190, 81, 174, 97, 158, 113, 142, 5, 250, 21, 234, 38, 217, 54, 201, 70, 185, 86, 169, 103, 152, 119, 136, 11, 244, 27, 228, 43, 212, 59, 196, 76, 179, 92, 163, 108, 147, 124, 131,
    0, 255, 8, 247, 16, 239, 24, 231, 32, 223, 40, 215, 48, 207, 56, 199, 64, 191, 72, 183, 80, 175, 88, 167, 96, 159, 104, 151, 112, 143, 120, 135, 2, 253, 10, 245, 18, 237, 26, 229, 35, 220, 43, 212, 51, 204, 59, 196, 67, 188, 75, 180, 83, 172, 91, 164, 99, 156, 107, 148, 115, 140, 123, 132, 5, 250, 13, 242, 21, 234, 29, 226, 37, 218, 45, 210, 53, 202, 61, 194, 70, 185, 78, 177, 86, 169, 94, 161, 102, 153, 110, 145, 118, 137, 126, 129,
    0, 255, 4, 251, 8, 247, 12, 243, 16, 239, 20, 235, 24, 231, 28, 227, 32, 223, 36, 219, 40, 215, 44, 211, 48, 207, 52, 203, 56, 199, 60, 195, 64, 191, 68, 187, 72, 183, 76, 179, 80, 175, 84, 171, 88, 167, 92, 163, 96, 159, 100, 155, 104, 151, 108, 147, 112, 143, 116, 139, 120, 135, 124, 131, 1, 254, 5, 250, 9, 246, 13, 242, 17, 238, 21, 234, 25, 230, 29, 226, 33, 222, 37, 218, 41, 214, 45, 210, 49, 206, 53, 202, 57, 198, 61, 194, 65, 190, 69, 186, 73, 182, 77, 178, 81, 174, 85, 170, 89, 166, 93, 162, 97, 158, 101, 154, 105, 150, 109, 146, 113, 142, 117, 138, 121, 134, 125, 130, 2, 253, 6, 249, 10, 245, 14, 241, 18, 237, 22, 233, 26, 229, 30, 225, 34, 221, 38, 217, 42, 213, 46, 209, 50, 205, 54, 201, 58, 197, 62, 193, 66, 189, 70, 185, 74, 181, 78, 177, 82, 173, 86, 169, 90, 165, 94, 161, 98, 157, 102, 153, 106, 149, 110, 145, 114, 141, 118, 137, 122, 133, 126, 129,
};

uint kUnquantQuintColorMapBitIdx[6] = {
    0, 5, 15, 35, 75, 155,
};

uint kUnquantQuintColorMap[315] = {
    0, 0, 0, 0, 0,
    0, 255, 28, 227, 56, 199, 84, 171, 113, 142,
    0, 255, 67, 188, 13, 242, 80, 175, 27, 228, 94, 161, 40, 215, 107, 148, 54, 201, 121, 134,
    0, 255, 32, 223, 65, 190, 97, 158, 6, 249, 39, 216, 71, 184, 104, 151, 13, 242, 45, 210, 78, 177, 110, 145, 19, 236, 52, 203, 84, 171, 117, 138, 26, 229, 58, 197, 91, 164, 123, 132,
    0, 255, 16, 239, 32, 223, 48, 207, 64, 191, 80, 175, 96, 159, 112, 143, 3, 252, 19, 236, 35, 220, 51, 204, 67, 188, 83, 172, 100, 155, 116, 139, 6, 249, 22, 233, 38, 217, 54, 201, 71, 184, 87, 168, 103, 152, 119, 136, 9, 246, 25, 230, 42, 213, 58, 197, 74, 181, 90, 165, 106, 149, 122, 133, 13, 242, 29, 226, 45, 210, 61, 194, 77, 178, 93, 162, 109, 146, 125, 130,
    0, 255, 8, 247, 16, 239, 24, 231, 32, 223, 40, 215, 48, 207, 56, 199, 64, 191, 72, 183, 80, 175, 88, 167, 96, 159, 104, 151, 112, 143, 120, 135, 1, 254, 9, 246, 17, 238, 25, 230, 33, 222, 41, 214, 49, 206, 57, 198, 65, 190, 73, 182, 81, 174, 89, 166, 97, 158, 105, 150, 113, 142, 121, 134, 3, 252, 11, 244, 19, 236, 27, 228, 35, 220, 43, 212, 51, 204, 59, 196, 67, 188, 75, 180, 83, 172, 91, 164, 99, 156, 107, 148, 115, 140, 123, 132, 4, 251, 12, 243, 20, 235, 28, 227, 36, 219, 44, 211, 52, 203, 60, 195, 68, 187, 76, 179, 84, 171, 92, 163, 100, 155, 108, 147, 116, 139, 124, 131, 6, 249, 14, 241, 22, 233, 30, 225, 38, 217, 46, 209, 54, 201, 62, 193, 70, 185, 78, 177, 86, 169, 94, 161, 102, 153, 110, 145, 118, 137, 126, 129,
};
"""
shader_sources = ["", ""]
file_name = os.path.join(shader_source_path, shader_lib_name + ".comp")
with open(file_name, 'r') as file:
    shader_sources[0] = file.read()

for shader_source_name in shader_source_names:
    file_name = os.path.join(shader_source_path, shader_source_name + ".comp")
    for image_type in image_types:
        with open(file_name, 'r') as file:
            shader_sources[1] = file.read()
        tmp_file, tmp_file_name = tempfile.mkstemp(suffix=".comp")
        os.close(tmp_file)
        if shader_source_name != "Astc":
            combined_shader_source = "\n".join(shader_sources)
        else:
            combined_shader_source = shader_sources[1]
        combined_shader_source = combined_shader_source.replace("${type}", image_type)
        combined_shader_source = combined_shader_source.replace("${UnquantTables}", unquant_table_string)

        print(image_type)

        source_file = open(tmp_file_name, 'w')
        source_file.write(combined_shader_source)
        source_file.close()

        out_spv_path = os.path.join(shader_source_path, shader_source_name + "_" + image_type + ".spv")
        if os.path.exists(out_spv_path):
            os.remove(out_spv_path)
        cmd = " ".join([glslangValidator_path, "--spirv-val", "--target-env", "vulkan1.1", "-V", "-Od", "-H", "-o", out_spv_path, tmp_file_name]);
        print(cmd)
        ret = os.system(cmd)
        if ret != 0:
            print("Compiling %s got return code %d" % (out_spv_path, ret))
            print("Please look at intermediate source file for debug: %s" % tmp_file_name)
            exit()
        else:
            print ("Compiled %s successfully" % out_spv_path)
            os.remove(tmp_file_name)
