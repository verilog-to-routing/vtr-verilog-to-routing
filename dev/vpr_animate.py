#!/usr/bin/env python3

import argparse
import sys

GRAPHICS_COMMAND = "set_draw_block_outlines 0; set_draw_block_text 0; save_graphics img_{i}_place.png; set_macros 1; save_graphics img_{i}_macros.png; set_macros 0; set_draw_net_max_fanout 1000; set_nets 1; save_graphics img_{i}_nets1.png; set_nets 2; save_graphics img_{i}_nets2.png; set_nets 0; set_cpd 1; save_graphics img_{i}_cpd1.png; set_cpd 2; save_graphics img_{i}_cpd2.png; set_cpd 0; set_routing_util 4; set_clip_routing_util 1; save_graphics img_{i}_routing_util4.png; set_routing_util 0; set_congestion 1; save_graphics img_{i}_congestion2.png; exit 0;"


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("vpr")
    parser.add_argument("arch")
    parser.add_argument("circuit")
    parser.add_argument("--gen_state", action="store_true", default=False)
    parser.add_argument("--gen_frames", action="store_true", default=False)
    parser.add_argument("--render", action="store_true", default=False)
    parser.add_argument("--placements", nargs="+")
    parser.add_argument("--routings", nargs="+")

    return parser.parse_known_args()


def main():
    args, extra_args = parse_args()

    vpr_base_cmd = "{} {} {} --route_chan_width 300 --max_router_iterations 400 --router_lookahead map --verify_file_digests off --save_graphics on".format(
        args.vpr, args.arch, args.circuit
    )

    for arg in extra_args:
        vpr_base_cmd += " " + arg

    if args.gen_state:
        vpr_cmd = (
            vpr_base_command
            + "--write_rr_graph rr_graph.bin --write_placement_delay_lookup placement_delay_lookup.bin --write_router_lookahead router_lookahead.bin"
        )

        print(vpr_cmd)

    if args.gen_frames:
        i = 0
        for placement in args.placements:
            cmd = (
                "/usr/bin/time -v "
                + vpr_base_cmd
                + " --place_file "
                + placement
                + " --read_rr_graph rr_graph.bin --read_placement_delay_lookup placement_delay_lookup.bin --read_router_lookahead router_lookahead.bin "
            )
            cmd += " --route"
            cmd += " --graphics_commands '{}'".format(GRAPHICS_COMMAND.format(i=i))
            cmd += " >& vpr_img_{}.log".format(i)
            cmd += " && echo 'Finished {}' || echo 'Failed {}'".format(i, i)
            print(cmd)
            i += 1

        for routing in args.routings:
            cmd = (
                "/usr/bin/time -v "
                + vpr_base_cmd
                + " --place_file "
                + args.placements[-1]
                + " --read_rr_graph rr_graph.bin --read_placement_delay_lookup placement_delay_lookup.bin --read_router_lookahead router_lookahead.bin "
            )
            cmd += " --route_file " + routing + " --analysis"
            cmd += " --graphics_commands '{}'".format(GRAPHICS_COMMAND.format(i=i))
            cmd += " >& vpr_img_{}.log".format(i)
            cmd += " && echo 'Finished {}' || echo 'Failed {}'".format(i, i)
            print(cmd)
            i += 1

    num_frames = len(args.placements) + len(args.routings)

    run_time = 20

    fps = num_frames / run_time

    if args.render:
        # cmd = "ffmpeg -y -pattern_type sequence -i 'img_%d_{inmetric}.png' -map_metadata -1 -vf scale=402:300:flags=lanczos -b:v 0 -crf 24 -f mp4 -vcodec libx264 -pix_fmt yuv420p -movflags +faststart -preset veryslow -tune animation -an -t 15 {out}.mp4"

        # Scale in raw format to lower resolution
        cmd_hq_yuv = "ffmpeg -y -framerate {fps} -pattern_type sequence -i 'img_%d_{inmetric}.png' -vf scale=640:480:flags=lanczos -vcodec rawvideo -pix_fmt yuv420p raw_hq_{out}.yuv"
        cmd_lq_yuv = "ffmpeg -y -framerate {fps} -pattern_type sequence -i 'img_%d_{inmetric}.png' -vf scale=250:186:flags=lanczos -vcodec rawvideo -pix_fmt yuv420p raw_lq_{out}.yuv"

        # Encode skipping every other frame (select=)
        cmd_hq = (
            cmd_hq_yuv
            + ' && ffmpeg -y -framerate {fps2} -s 640x480 -i raw_hq_{out}.yuv -b:v 0 -crf 24 -f mp4 -vcodec libx264 -pix_fmt yuv420p -movflags +faststart -preset veryslow -tune animation -an -ss 0 -filter:v select="mod(n-1\,2)" {out}.mp4'
        )
        cmd_mq = (
            cmd_hq_yuv
            + ' && ffmpeg -y -framerate {fps2} -s 640x480 -i raw_hq_{out}.yuv -b:v 0 -crf 28 -f mp4 -vcodec libx264 -pix_fmt yuv420p -movflags +faststart -preset veryslow -tune animation -an -ss 0 -filter:v select="mod(n-1\,2)" {out}.mp4'
        )
        cmd_lq = (
            cmd_hq_yuv
            + ' && ffmpeg -y -framerate {fps2} -s 640x480 -i raw_hq_{out}.yuv -b:v 0 -crf 34 -f mp4 -vcodec libx264 -pix_fmt yuv420p -movflags +faststart -preset veryslow -tune animation -an -ss 0 -filter:v select="mod(n-1\,2)" {out}.mp4'
        )

        # cmd_hq = "ffmpeg -y -framerate {fps} -pattern_type sequence -i 'img_%d_{inmetric}.png' -map_metadata -1 -vf scale=536:400:flags=lanczos -b:v 0 -crf 24 -f mp4 -vcodec libx264 -pix_fmt yuv420p -movflags +faststart -preset veryslow -tune animation -an -ss 0 {out}.mp4"
        # cmd_mq = "ffmpeg -y -framerate {fps} -pattern_type sequence -i 'img_%d_{inmetric}.png' -map_metadata -1 -vf scale=536:400:flags=lanczos -b:v 0 -crf 28 -f mp4 -vcodec libx264 -pix_fmt yuv420p -movflags +faststart -preset veryslow -tune animation -an -ss 0 {out}.mp4"
        # cmd_lq = "ffmpeg -y -framerate {fps} -pattern_type sequence -i 'img_%d_{inmetric}.png' -map_metadata -1 -vf scale=536:400:flags=lanczos -b:v 0 -crf 34 -f mp4 -vcodec libx264 -pix_fmt yuv420p -movflags +faststart -preset veryslow -tune animation -an -ss 0 {out}.mp4"

        # For GIF use only half of frames (select) at lower resolution and skip 2 seconds (-ss 2) at beginning
        cmd_gif = "ffmpeg -y -framerate {fps} -pattern_type sequence -i 'img_%d_{inmetric}.png' -vf scale=250:186:flags=lanczos -vcodec rawvideo -pix_fmt yuv420p raw_gif_{out}.yuv && ffmpeg -y -framerate {fps2} -s 250x186 -i raw_gif_{out}.yuv -ss 2 -filter:v select=\"mod(n-1\,2)\" {out}.gif"

        # AV1
        # cmd = "ffmpeg -y -framerate 30 -pattern_type sequence -i 'img_%d_{inmetric}.png' -map_metadata -1 -vf scale=644:480 -b:v 0 -crf 50 -f mp4 -c:v libaom-av1 -pix_fmt yuv420p -movflags +faststart -strict experimental {out}.av1.mp4"

        for inval, outval in [
            ("macros", "placement_macros"),
            ("cpd1", "cpd"),
            ("routing_util4", "routing_util"),
            ("nets2", "nets"),
        ]:
            if "nets" in inval:
                print(cmd_lq.format(inmetric=inval, out=outval, fps=fps, fps2=2 * fps))
            elif "routing_util" in inval:
                print(cmd_mq.format(inmetric=inval, out=outval, fps=fps, fps2=2 * fps))
            else:
                print(cmd_hq.format(inmetric=inval, out=outval, fps=fps, fps2=2 * fps))
            print(cmd_gif.format(inmetric=inval, out=outval, fps=fps, fps2=2 * fps))


if __name__ == "__main__":
    main()
