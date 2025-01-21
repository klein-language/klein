use proc_macro::TokenStream;

/// Run a string of Klein code, validating that it's properly formatted at compile-time.
#[proc_macro]
pub fn run_static(input: TokenStream) -> TokenStream {
    // Store input tokens
    let input_tokens: proc_macro2::TokenStream = input.clone().into();

    // Validate parsing
    let string = syn::parse_macro_input!(input as syn::LitStr).value();
    if let Some(error) = cklein_core::check(&string) {
        panic!("Error in Klein code: {error}");
    }

    // Return the run function
    quote::quote! {
        ::cklein::run(#input_tokens).map_err(|error| {
            let ::cklein::KleinError::RuntimeError(runtime_error) = error else { unreachable!() };
            runtime_error
        })
    }
    .into()
}
