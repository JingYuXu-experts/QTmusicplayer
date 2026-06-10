import os

# ================= 配置区域 =================
ALLOWED_EXTENSIONS = {
    '.cpp', '.h',        # C++ 核心代码
    '.qml',              # QML 界面代码
    '.txt',              # 主要是 CMakeLists.txt
    '.qrc',              # 资源配置文件
    '.lrc',              # 歌词文件
    '.json', '.conf'     # 配置文件
}

IGNORE_DIRS = {
    'build', 'build_debug', 'build_release', 
    'debug', 'release', 'bin', 'obj', 'moc', 
    '.git', '.vscode', '.idea', 'cmake-build-debug'
}

OUTPUT_FILE = 'applemusic_context.txt'
# ===========================================

def is_source_file(filename):
    return any(filename.endswith(ext) for ext in ALLOWED_EXTENSIONS)

def merge_files(start_path):
    file_count = 0
    
    print(f"\n--- 开始扫描 ---")
    print(f"扫描根目录: {start_path}")
    
    try:
        with open(OUTPUT_FILE, 'w', encoding='utf-8') as outfile:
            for root, dirs, files in os.walk(start_path):
                # 打印当前正在扫描的文件夹
                print(f"正在扫描文件夹: {root}")
                
                # 过滤并打印被忽略的文件夹
                original_dirs = list(dirs)
                dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
                for d in original_dirs:
                    if d in IGNORE_DIRS:
                        print(f"  [忽略文件夹] {d}")

                # 扫描文件
                for filename in files:
                    file_path = os.path.join(root, filename)
                    
                    if is_source_file(filename):
                        try:
                            # 尝试读取文件
                            with open(file_path, 'r', encoding='utf-8', errors='ignore') as infile:
                                content = infile.read()
                                
                                # 写入文件
                                outfile.write(f"\n{'='*30} FILE: {filename} {'='*30}\n")
                                outfile.write(content)
                                outfile.write(f"\n{'='*30} END {'='*30}\n")
                                
                                print(f"  [√ 写入成功] {filename}")
                                file_count += 1
                        except Exception as e:
                            print(f"  [× 读取错误] {filename}: {e}")
                    else:
                        # 如果你想看哪些文件被跳过了，可以取消下面这行的注释
                        # print(f"  [跳过类型] {filename}")
                        pass

    except Exception as e:
        print(f"\n!!! 发生严重错误: {e}")

    return file_count

if __name__ == "__main__":
    current_dir = os.getcwd()
    # 强制让脚本扫描脚本所在的目录（防止你在错误的终端路径下运行）
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    print(f"脚本所在目录: {script_dir}")
    print(f"当前工作目录: {current_dir}")
    
    # 优先扫描脚本所在的目录
    count = merge_files(script_dir)
    
    print(f"\n--- 扫描结束 ---")
    if count == 0:
        print("❌ 没有找到任何符合条件的文件！")
        print("可能的原因：")
        print("1. 你把脚本放错文件夹了（请确保它和 main.cpp 在同一个文件夹，或者在它们的上一级）。")
        print("2. 你的代码文件后缀不在 ALLOWED_EXTENSIONS 列表里。")
    else:
        print(f"✅ 成功处理了 {count} 个文件。")
        print(f"结果文件保存在: {os.path.join(script_dir, OUTPUT_FILE)}")