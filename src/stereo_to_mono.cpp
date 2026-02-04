struct StereoToMono {
    auto process(float in_left, float in_right) -> float {
        return (in_left + in_right) * 0.7071;
    }
};
