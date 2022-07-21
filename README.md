# Barebones OpenXR OpenGL

[Video footage](https://www.youtube.com/watch?v=gScJ7H8TbW0)

## Code Structure Overview
```
main()
    openxr_init()
    opengl_init()
    game_loop
        openxr_poll_events()
        openxr_render_frame();
    openxr_shutdown();
    opengl_shutdown();

openxr_init()
    Query and setup extensions
    Create instance, system
    device_init()
        Setup graphics api (OpenGL)
        Create app window (drawing context)
        Bind graphics api to context
    Create session
    Create reference space
    Create view configuration (form factor, eg. mono/stereo)
    Create swap chain

opengl_init()
    Setup and bind shaders
    Setup and bind vertex arrays

openxr_poll_events()
    loop xrPollEvent()
        Detect for session state changes and process accordingly

openxr_render_frame()
    loop through view_count
        prepare view (swap chain image)
        openxr_render_layer()
            opengl_render_layer()
                gl draw calls
            release swap chaim image

*shutdown()
    delete stuff
```