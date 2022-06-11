idf_component_register(
    SRCS
        "main.c"
        "wifi_connect.c"
        "wifi_connection.c"
    INCLUDE_DIRS
        "." "include"
    EMBED_FILES ${project_dir}/resources/nick.png
    EMBED_FILES ${project_dir}/resources/nick_a.png
    EMBED_FILES ${project_dir}/resources/nick_gear.png
)