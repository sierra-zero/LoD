// Copyright (c) 2014, Tamas Csala

#version 430

in vec2 vTexCoord;

uniform sampler2D uTex;

out vec4 fragColor;

void main() {
  fragColor = vec4(texture2D(uTex, vTexCoord).rgb, 1.0);
}
