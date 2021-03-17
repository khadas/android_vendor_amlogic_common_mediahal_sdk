package mediahalvercheck

import (
    "android/soong/android"
    "android/soong/cc"
    "fmt"
    "github.com/google/blueprint/proptools"
    "strconv"
)

func MediahalVerCheckDefaults(ctx android.LoadHookContext) {
    sdkVersion := ctx.DeviceConfig().VndkVersion()
    sdkVersionInt,err := strconv.Atoi(sdkVersion)
    if err != nil {
        fmt.Printf("%v fail to convert", sdkVersionInt)
    } else {
        fmt.Println("sdkVersion:", sdkVersionInt)
    }
    if sdkVersionInt >= 30 {
        type props struct {
            System_ext_specific  *bool
        }
        p := &props{}
        p.System_ext_specific = proptools.BoolPtr(true)
        ctx.AppendProperties(p)
    }
}

func init() {
    android.RegisterModuleType("mediahalvercheck_defaults", MediahalVerCheckDefaultsFactory)
}

func MediahalVerCheckDefaultsFactory() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, MediahalVerCheckDefaults)

    return module
}