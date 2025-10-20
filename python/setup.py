from setuptools import find_packages, setup

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="dinorisc",
    version="1.0.0",
    author="DinoRISC Contributors",
    description="Python API for DinoRISC - RISC-V to ARM64 dynamic binary translator",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/yourusername/dinorisc",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Operating System :: POSIX :: Linux",
        "Operating System :: MacOS",
        "Topic :: Software Development :: Compilers",
    ],
    python_requires=">=3.7",
    install_requires=[],
)
