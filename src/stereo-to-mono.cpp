struct StereoToMono{
    float process(float in_left, float in_right) {
        return (in_left + in_right) * 0.7071;
    }
};
