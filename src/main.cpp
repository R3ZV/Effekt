#include <jack/jack.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>

jack_port_t *input_port;
jack_port_t *output_port;

auto process(jack_nframes_t nframes, void *arg) -> int {
    auto *in = (jack_default_audio_sample_t *)jack_port_get_buffer(input_port,
                                                                   nframes);
    auto *out = (jack_default_audio_sample_t *)jack_port_get_buffer(output_port,
                                                                    nframes);

    for (int i = 0; i < nframes; i++) {
        out[i] = in[i] * 5.0f;
    }

    return EXIT_SUCCESS;
}

auto jack_shutdown(void *arg) -> void {
    std::cerr << "JACK server shut down. Exiting..." << std::endl;
    exit(EXIT_FAILURE);
}

auto main(int argc, char *argv[]) -> int {
    const char *client_name = "Effekt AMP";
    const char *server_name = nullptr;
    jack_options_t options = JackNullOption;
    jack_status_t status;

    jack_client_t *client =
        jack_client_open(client_name, options, &status, server_name);

    if (client == nullptr) {
        std::cerr << "jack_client_open() failed, status = 0x" << std::hex
                  << status << std::endl;
        if (status & JackServerFailed) {
            std::cerr << "Unable to connect to JACK server." << std::endl;
        }
        return 1;
    }

    if (status & JackServerStarted) {
        std::cout << "JACK server started." << std::endl;
    }

    if (status & JackNameNotUnique) {
        client_name = jack_get_client_name(client);
        std::cout << "Unique name assigned: " << client_name << std::endl;
    }

    jack_set_process_callback(client, process, nullptr);
    jack_on_shutdown(client, jack_shutdown, nullptr);

    input_port = jack_port_register(
        client, "guitar_in", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    output_port = jack_port_register(client, "amp_out", JACK_DEFAULT_AUDIO_TYPE,
                                     JackPortIsOutput, 0);

    if (input_port == nullptr || output_port == nullptr) {
        std::cerr << "no more JACK ports available" << std::endl;
        return 1;
    }

    if (jack_activate(client)) {
        std::cerr << "cannot activate client" << std::endl;
        return 1;
    }

    std::cout << "Client '" << client_name << "' is active." << std::endl;
    std::cout << "Connect your system capture (guitar) to " << client_name
              << ":guitar_in" << std::endl;
    std::cout << "Connect " << client_name
              << ":amp_out to your system playback." << std::endl;
    std::cout << "Press Ctrl+C to quit..." << std::endl;

    while (true) {
        sleep(1);
    }

    jack_client_close(client);
    return 0;
}
