PROJECT_SOURCE_DIR=$(cd $(dirname ${BASH_SOURCE[0]:-${(%):-%x}})/; pwd)
PROJECT_BINARY_DIR=${PROJECT_SOURCE_DIR}/build

# default build tool
build_tool="make"

docker_image="opengl-tutorial"

#  "docker build"
dkb() {
    echo "running dkb ..."

    CMD="docker run -it --rm\
        --gpus all
        -v ${HOME}/.Xauthority:/home/user/.Xauthority \
        -v $PROJECT_SOURCE_DIR:$PROJECT_SOURCE_DIR -v $PROJECT_BINARY_DIR:$PROJECT_BINARY_DIR \
        -w `realpath $PWD` -u $(id -u):$(id -g)\
        -e DISPLAY \
        --net=host \
        $docker_image \
        bash -c \"$*\""

    echo ${CMD}

    eval ${CMD}
}
