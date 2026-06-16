#version 410 core
//
// shadow_depth.frag - OLE-04: minimalny fragment shader dla przebiegu cieni.
//
// Nie musimy nic pisac do bufora koloru - OpenGL automatycznie zapisuje
// gl_FragCoord.z do GL_DEPTH_COMPONENT. Pusty main() wystarczy.
//

void main()
{
    // gl_FragDepth automatycznie ustawiane na gl_FragCoord.z
}
